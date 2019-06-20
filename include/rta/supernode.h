#ifndef SUPERNODE_H
#define SUPERNODE_H

#include <crypto/crypto.h>
#include <cryptonote_config.h>
#include <graft_rta_config.h>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/asio/io_service.hpp>
#include <string>
#include <vector>

namespace tools {
    class wallet2;
}

namespace cryptonote {
    class transaction;
}

namespace graft {

/*!
 * \brief Supernode stake description
 */
struct supernode_stake
{
  uint64_t amount = 0;
  uint64_t block_height = 0;
  uint64_t unlock_time = 0;
  std::string supernode_public_id;
  std::string supernode_public_address;
};

/*!
 * \brief The Supernode class - Representing supernode instance
 */
class Supernode
{
public:
    using SignedKeyImage = std::pair<crypto::key_image, crypto::signature>;

    static constexpr uint64_t TIER1_STAKE_AMOUNT = config::graft::TIER1_STAKE_AMOUNT;
    static constexpr uint64_t TIER2_STAKE_AMOUNT = config::graft::TIER2_STAKE_AMOUNT;
    static constexpr uint64_t TIER3_STAKE_AMOUNT = config::graft::TIER3_STAKE_AMOUNT;
    static constexpr uint64_t TIER4_STAKE_AMOUNT = config::graft::TIER4_STAKE_AMOUNT;

    Supernode(const std::string &wallet_address, const crypto::public_key &id_key, const std::string &daemon_address, bool testnet = false);

    ~Supernode();

    /*!
     * \brief refresh         - get latest blocks from the daemon
     * \return                - true on success
     */
    bool refresh();

    /*!
     * \brief testnet        - to check if wallet is testnet wallet
     * \return               - true if testnet
     */
    bool testnet() const;


    /*!
     * \brief stakeAmount - returns stake amount, i.e. the wallet balance that only counts verified-unspent inputs.
     * \return            - stake amount in atomic units
     */
    uint64_t stakeAmount() const;

    /*!
     * \brief tier - returns the tier of this supernode based on its stake amount
     * \return     - the tier (1-4) of the supernode or 0 if the verified stake amount is below tier 1
     */
    uint32_t tier() const;
    /*!
     * \brief walletAddress - returns wallet address as string
     * \return
     */
    std::string walletAddress() const;

    /*!
     * \brief setWalletAddress - sets wallet public address
     */
    void setWalletAddress(const std::string &address);

    /*!
     * \brief createFromStake - creates new Supernode instance from a stake
     * \param stake           - stake of the supernode
     * \param testnet         - testnet flag
     * \return                - Supernode pointer on success
     */
    static Supernode * createFromStake(const supernode_stake& stake,
                                       const std::string &daemon_address,
                                       bool testnet);

    /*!
     * \brief signMessage - signs message. internally hashes the message and signs the hash
     * \param msg         - input message
     * \param signature   - output signature
     * \return            - true if successfully signed. false if wallet is watch-only
     */
    bool signMessage(const std::string &msg, crypto::signature &signature) const;


    bool signHash(const crypto::hash &hash, crypto::signature &signature) const;
    /*!
     * \brief verifySignature - verifies signature
     * \param msg             - message to verify
     * \param signature       - signer's signature
     * \return                - true if signature valid
     */
    static bool verifySignature(const std::string &msg, const crypto::public_key &pkey, const crypto::signature &signature);

    /*!
     * \brief getScoreHash  - calculates supernode score (TODO: as 265-bit integer)
     * \param block_hash    - block hash used in calculation
     * \param result        - result will be written here;
     * \return              - true on success
     */

    static bool verifyHash(const crypto::hash &hash, const crypto::public_key &pkey, const crypto::signature &signature);


    void getScoreHash(const crypto::hash &block_hash, crypto::hash &result) const;

    std::string networkAddress() const;

    void setNetworkAddress(const std::string &networkAddress);

    /*!
     * \brief getAmountFromTx - scans given tx for outputs destined to this address
     * \param tx              - transaction object
     * \param amount          - amount in atomic units
     * \return                - true on success (only indicates error, amount still can be zero)
     */
    bool getAmountFromTx(const cryptonote::transaction &tx, uint64_t &amount);

    // TODO: implement me. see cryptonode/src/supernode/grafttxextra.h how payment id can be put/get from transaction object
    /*!
     * \brief getPaymentIdFromTx - returns graft payment id from given transaction
     * \param tx                 - transaction object
     * \param paymentId          - output
     * \return                   - true on success. false if some error or payment id not found in tx
     */
    bool getPaymentIdFromTx(const cryptonote::transaction &tx, std::string &paymentId);

    /*!
     * \brief validateAddress - validates wallet address
     * \param address         - address to validate
     * \param testnet         - testnet flag
     * \return                - true if address valid
     */

    static bool validateAddress(const std::string &address, bool testnet);

    /*!
     * \brief lastUpdateTime - returns timestamp when supernode updated last time
     * \return
     */
    int64_t lastUpdateTime() const;

    /*!
     * \brief setLastUpdateTime - updates wallet refresh time
     * \param time
     */
    void setLastUpdateTime(int64_t time);

    /*!
     * \brief busy - checks if stake wallet currently busy
     * \return
     */
    bool busy() const;

    /*!
     * \brief stakeBlockHeight - height of block for stake
     * \return                 - height of block
     */
    uint64_t stakeBlockHeight() const;

    /*!
     * \brief stakeUnlockTime - number of blocks for unlocking stake
     * \return
     */
    uint64_t stakeUnlockTime() const;

    /*!
     * \brief setStake - set stake details
     * \param          - stake amount
     * \param          - height of block
     * \param          - unlock time
     */
    void setStake(uint64_t amount, uint64_t blockHeight, uint64_t unlockTime);

    /*!
     * \brief loadKeys
     * \param filename
     * \return
     */
    bool loadKeys(const std::string &filename);

    /*!
     * \brief initKeys
     * \param force
     * \return
     */
    void initKeys();

    bool saveKeys(const std::string &filename, bool force = false);

    const crypto::public_key &idKey() const;
    const crypto::secret_key &secretKey() const;
    std::string idKeyAsString() const;


private:
    Supernode(bool testnet = false);

private:
    // wallet's address. empty in case 'their' supernode
    mutable boost::shared_mutex m_access;
    std::string           m_wallet_address;
    crypto::public_key    m_id_key;
    crypto::secret_key    m_secret_key;
    bool                  m_has_secret_key = false;
    std::atomic<int64_t>  m_last_update_time;
    uint64_t              m_stake_amount;
    uint64_t              m_stake_block_height;
    uint64_t              m_stake_unlock_time;
    bool                  m_testnet = false;
    std::string           m_network_address;
};

using SupernodePtr = boost::shared_ptr<Supernode>;

} // namespace

#endif // SUPERNODE_H
