#pragma once

#include "SharedMemoryClient.h"
#include "SharedMemoryServer.h"
#include <boost/asio.hpp>
#include "Export.h"

using boost::asio::ip::tcp;
using namespace boost;
using namespace boost::asio;
struct ReplicateTopicMessage {
    uint32_t TopicNameLength;
    // Topic name follows as char array
};
struct ReplicationMessage {
    uint32_t Size;
    uint64_t Type;
    // Data follows
};
class ShmReplicationTarget;

/// <summary>
/// For scenarios where we don't spin ShmReplicationTarget in a separate process.
/// </summary>
class EXPORT ShmReplicationClient : ISharedMemoryClient
{
public:
	ShmReplicationClient(const std::string &channel, const std::string& host, uint16_t port) {  }
    void Connect() override { }
    std::unique_ptr<ISubscriptionCursor> Subscribe(const std::string& topicName) override
	{
        throw std::exception("foo");
	}
private:
    std::shared_ptr<SharedMemoryServer> _server;
    std::unordered_map<std::string, std::shared_ptr<ShmReplicationTarget>> _targets;

};
/// <summary>
/// For scenarios where you want to replicate channels topic's over TCP.
/// </summary>
class EXPORT ShmReplicationSource {
private:
    struct TopicReplicator {
        std::string TopicName;
        std::unique_ptr<ISubscriptionCursor> Cursor;
        std::thread ReplicationThread;
        std::atomic<bool> Running{ true };
    };

    boost::asio::io_context& _io;
    tcp::acceptor _acceptor;
    SharedMemoryClient _shmClient;
    std::unordered_map<std::shared_ptr<tcp::socket>, std::vector<std::shared_ptr<TopicReplicator>>> _clientTopics;
    std::atomic<bool> _running{ true };
    std::mutex _clientsMutex;

    void AcceptLoop();
    void HandleNewClient(std::shared_ptr<tcp::socket> socket);
    void ReplicateLoop(std::shared_ptr<tcp::socket> socket, std::shared_ptr<TopicReplicator> replicator);
    void HandleReplicateSubscription(std::shared_ptr<tcp::socket> socket);

public:
    ShmReplicationSource(asio::io_context& io, const std::string& channelName,
         uint16_t port);

    ~ShmReplicationSource();
};

/// <summary>
/// For most scenarios, we spin ShmReplicatorTarget on a host that should replicate memory from the server
/// just as if we were communicating through bare Shm using SharedMemoryServer and SharedMemoryClient.
/// </summary>
class EXPORT ShmReplicationTarget {
private:
    struct TopicReplicator {
        std::string TopicName;
        std::thread ReplicationThread;
        std::atomic<bool> Running{ true };
    };

    asio::io_context& _io;
    tcp::socket _socket;
    std::shared_ptr<SharedMemoryServer> _shmServer;
    std::vector<std::shared_ptr<TopicReplicator>> _replicators;
    std::atomic<bool> _running{ true };
    std::mutex _replicatorsMutex;

    
    void ReplicateLoop(std::shared_ptr<TopicReplicator> replicator);
    void StartReplication(const std::string& topicName);

public:
    ShmReplicationTarget(asio::io_context& io, 
        std::shared_ptr<SharedMemoryServer> shmServer,
        
        const std::string& host, uint16_t port);

    void ReplicateTopic(const std::string& topicName);
    ~ShmReplicationTarget();
};