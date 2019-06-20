#include "supernode/supernode.h"
#include "rta/fullsupernodelist.h"

#include <misc_log_ex.h>
#include <wallet/wallet2.h>
#include <cryptonote_basic/cryptonote_basic_impl.h>
#include <boost/filesystem.hpp>
#include <boost/thread/locks.hpp>
#include <iostream>
#include <ctime>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "supernode.supernode"

using namespace std;

namespace graft {

#ifndef __cpp_inline_variables
constexpr uint64_t Supernode::TIER1_STAKE_AMOUNT, Supernode::TIER2_STAKE_AMOUNT, Supernode::TIER3_STAKE_AMOUNT, Supernode::TIER4_STAKE_AMOUNT;
#endif

Supernode::Supernode(const string &wallet_address, const crypto::public_key &id_key, const string &daemon_address, bool testnet)
    : m_wallet_address(wallet_address)
    , m_id_key(id_key)
    , m_has_secret_key(false)
    , m_last_update_time {0}
    , m_stake_amount()
    , m_stake_block_height()
    , m_stake_unlock_time()
    , m_testnet(testnet)
{
    MINFO("supernode created: " << "[" << this << "] " <<  this->walletAddress() << ", " << this->idKeyAsString());
}


Supernode::~Supernode()
{
    LOG_PRINT_L0("destroying supernode: " << "[" << this << "] " <<  this->walletAddress() << ", " << this->idKeyAsString());
}

uint64_t Supernode::stakeAmount() const
{
    boost::shared_lock<boost::shared_mutex> readerLock(m_access);
    return m_stake_amount;
}

uint32_t Supernode::tier() const
{
    auto stake = stakeAmount();
    return 0 +
        (stake >= TIER1_STAKE_AMOUNT) +
        (stake >= TIER2_STAKE_AMOUNT) +
        (stake >= TIER3_STAKE_AMOUNT) +
        (stake >= TIER4_STAKE_AMOUNT);
}

string Supernode::walletAddress() const
{
    boost::shared_lock<boost::shared_mutex> readerLock(m_access);
    return m_wallet_address;
}

void Supernode::setWalletAddress(const std::string &address)
{
    boost::unique_lock<boost::shared_mutex> writerLock(m_access);

    m_wallet_address = address;
}

Supernode* Supernode::createFromStake(const supernode_stake& stake, const std::string &daemon_address, bool testnet)
{
    crypto::public_key id_key;
    if (!epee::string_tools::hex_to_pod(stake.supernode_public_id, id_key)) {
        MERROR("Failed to parse id key from stake: " << stake.supernode_public_id);
        return nullptr;
    }

    std::unique_ptr<Supernode> result (new Supernode(stake.supernode_public_address, id_key, daemon_address, testnet));

    result->setLastUpdateTime(time(nullptr));
    result->setStake(stake.amount, stake.block_height, stake.unlock_time);

    return result.release();
}

bool Supernode::signMessage(const string &msg, crypto::signature &signature) const
{
    crypto::hash hash;

    crypto::cn_fast_hash(msg.data(), msg.size(), hash);
    MDEBUG("signing message: " << msg << ", hash: " << hash);
    return this->signHash(hash, signature);
}

bool Supernode::signHash(const crypto::hash &hash, crypto::signature &signature) const
{
    if (!m_has_secret_key) {
        LOG_ERROR("Attempting to sign with without private key");
        return false;
    }

    crypto::generate_signature(hash, m_id_key, m_secret_key, signature);
    return true;
}

bool Supernode::verifySignature(const string &msg, const crypto::public_key &pkey, const crypto::signature &signature)
{
    crypto::hash hash;
    crypto::cn_fast_hash(msg.data(), msg.size(), hash);
    return verifyHash(hash, pkey, signature);
}

bool Supernode::verifyHash(const crypto::hash &hash, const crypto::public_key &pkey, const crypto::signature &signature)
{
    return crypto::check_signature(hash, pkey, signature);
}

bool Supernode::refresh()
{
    MDEBUG("account refreshed: " << this->walletAddress());
    return true;
}

bool Supernode::testnet() const
{
    return m_testnet;
}

void Supernode::getScoreHash(const crypto::hash &block_hash, crypto::hash &result) const
{
    cryptonote::blobdata data = epee::string_tools::pod_to_hex(m_id_key);
    data += epee::string_tools::pod_to_hex(block_hash);
    crypto::cn_fast_hash(data.c_str(), data.size(), result);
}

string Supernode::networkAddress() const
{
    // boost::shared_lock<boost::shared_mutex> readLock(m_wallet_guard);
    return m_network_address;
}

void Supernode::setNetworkAddress(const string &networkAddress)
{
    if (m_network_address != networkAddress)
        m_network_address = networkAddress;
}

bool Supernode::getAmountFromTx(const cryptonote::transaction &tx, uint64_t &amount)
{
    // return m_wallet->get_amount_from_tx(tx, amount);
    return false;
}

bool Supernode::getPaymentIdFromTx(const cryptonote::transaction &tx, string &paymentId)
{
    return false;
}

bool Supernode::validateAddress(const string &address, bool testnet)
{
    cryptonote::address_parse_info addr_parse_info;
    cryptonote::network_type net_type = testnet ? cryptonote::TESTNET : cryptonote::MAINNET;
    return address.size() > 0 && cryptonote::get_account_address_from_str(addr_parse_info, net_type, address);
}

int64_t Supernode::lastUpdateTime() const
{
    return m_last_update_time;
}

void Supernode::setLastUpdateTime(int64_t time)
{
    m_last_update_time.store(time);
}

bool Supernode::busy() const
{
    return false;
}

uint64_t Supernode::stakeBlockHeight() const
{
    boost::shared_lock<boost::shared_mutex> readerLock(m_access);
    return m_stake_block_height;
}

void Supernode::setStake(uint64_t stakeAmount, uint64_t blockHeight, uint64_t unlockTime)
{
    boost::unique_lock<boost::shared_mutex> writerLock(m_access);

    m_stake_amount = stakeAmount;
    m_stake_block_height = blockHeight;
    m_stake_unlock_time = unlockTime;
}

uint64_t Supernode::stakeUnlockTime() const
{
    boost::shared_lock<boost::shared_mutex> readerLock(m_access);
    return m_stake_unlock_time;
}

bool Supernode::loadKeys(const string &filename)
{
    if (!boost::filesystem::exists(filename)) {
        MWARNING("failed to load keys from file: " << filename << ", file doesn't exists");
        return false;
    }

    MTRACE(" Reading wallet keys file '" << filename << "'");

    std::string key_data;
    if (!epee::file_io_utils::load_file_to_string(filename, key_data)) {
        MERROR("failed to load keys from file: " << filename << ", read error");
        return false;
    }

    if (!epee::string_tools::hex_to_pod(key_data, m_secret_key)) {
        MERROR("failed to load keys from file: " << filename << ", parse error");
        return false;
    }

    if (!crypto::secret_key_to_public_key(m_secret_key, m_id_key)) {
        MERROR("failed to load keys from file: " << filename << ", can't generate public key");
        return false;
    }
    m_has_secret_key = true;
    return true;
}


void graft::Supernode::initKeys()
{
    crypto::generate_keys(m_id_key, m_secret_key);
    m_has_secret_key = true;
}


bool Supernode::saveKeys(const string &filename, bool force)
{
    if (boost::filesystem::exists(filename) && !force) {
        MWARNING("key file already exists: " << filename << " but overwrite is not forced");
        return false;
    }

    //save secret key
    boost::filesystem::path wallet_keys_file_tmp = filename;
    wallet_keys_file_tmp += ".tmp";
    std::string data = epee::string_tools::pod_to_hex(m_secret_key);
    if (!epee::file_io_utils::save_string_to_file(wallet_keys_file_tmp.string(), data)) {
        MERROR("Cannot write to file '" << wallet_keys_file_tmp << "'");
        return false;
    }

    boost::system::error_code errcode;
    boost::filesystem::rename(wallet_keys_file_tmp, filename, errcode);
    if (errcode) {
        MERROR("Cannot rename '" << wallet_keys_file_tmp.string() << " to '" << filename  << "', :" << errcode.message());
        return false;
    }
    return true;
}

const crypto::public_key &Supernode::idKey() const
{
    return m_id_key;
}

const crypto::secret_key &Supernode::secretKey() const
{
    return m_secret_key;
}

std::string Supernode::idKeyAsString() const
{
    return epee::string_tools::pod_to_hex(m_id_key);
}

} // namespace graft

