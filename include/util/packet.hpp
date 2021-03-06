#pragma once

#include <cstdint>
#include <string>
#include <cstring>
#include <stdexcept>
#include <util/net_interface.h>

enum packet_type : uint32_t {
    GET,
    SEND,
    ERROR
};

struct packet {
    packet_type const p_type;
    
    packet(packet_type type)
    : p_type(type)
    {}

    virtual void* serialise(size_t& size) const = 0;

    /**
     * Writes the packet to a socket.
     * @param iface The network interface on which to send this packet.
     */
    bool send(net_interface& iface) const {
        size_t packet_size;
        void* serialised = this->serialise(packet_size);
        bool ret = true;
        try {
            iface.send(serialised, packet_size);
        } catch(net_interface::error& e) {
            std::cerr << "Error while sending packet: " << e.what() << std::endl;
            ret = false;
        }
        
        free(serialised);
        return ret;
    }
};

/**
 * Packet specifying a file to be sent to the remote host.
 */
struct send_packet : public packet {
    uint32_t name_size;
    char* name;

    uint32_t file_size;

    // Constructor for the sending side
    send_packet(std::string const& f_name, uint32_t f_size)
    : packet(SEND), name(new char[f_name.size() + 1]), name_size(f_name.size() + 1), file_size(f_size) {
        std::strcpy(this->name, f_name.c_str());
    }

    /**
     * Deserialises a send_packet from the given socket.
     * @param sock The socket from which to read the send_packet.
     */
    send_packet(net_interface& iface)
    : send_packet() {
        iface.receive(&this->name_size, sizeof(uint32_t));
        this->name = new char[this->name_size];
        iface.receive(this->name, this->name_size);
        iface.receive(&this->file_size, sizeof(uint32_t));
    }

    // Default constructor for the receiving side
    send_packet() 
    : packet(SEND), name(nullptr) {}

    ~send_packet() {
        delete[] name;
    }

    // Can we all just agree not to copy any of these
    send_packet(send_packet& other) = delete;

    virtual void* serialise(size_t& size) const {
        size = (sizeof(uint32_t) * 2) + sizeof(packet_type) + name_size;
        unsigned char* buf = (unsigned char*)malloc(size);
        size_t offset = 0;

        memcpy(buf + offset, &this->p_type, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, &this->name_size, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, this->name, this->name_size);
        offset += this->name_size;
        memcpy(buf + offset, &this->file_size, sizeof(uint32_t));
        return buf;
    }

};

/**
 * Packet specifying a file to be retrieved from the remote host.
 */
struct get_packet : public packet {   
    uint32_t name_size;
    char* name;

    get_packet(std::string const& file_name)
    : packet(GET), name(new char[file_name.size() + 1]), name_size(file_name.size() + 1) {
        std::strcpy(name, file_name.c_str());
    }

    get_packet(net_interface& iface)
    : get_packet() {
        iface.receive(&this->name_size, sizeof(uint32_t));        
        this->name = new char[this->name_size];
        iface.receive(this->name, this->name_size);
    }

    get_packet()
    : packet(GET), name(nullptr) {}

    ~get_packet() {
        delete[] name;
    }

    get_packet(get_packet& other) = delete;

    virtual void* serialise(size_t& size) const {
        size = sizeof(uint32_t) + this->name_size + sizeof(this->p_type);

        size_t offset = 0;
        unsigned char* buf = (unsigned char*)malloc(size);
        memcpy(buf + offset, &this->p_type, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, &this->name_size, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, this->name, this->name_size);
        return buf;
    }
};

/**
 * Packet signaling an error (sent from server to client).
 */
struct error_packet : public packet {
    packet_type p_type = ERROR;

    uint32_t err_size;
    char* err;

    error_packet(std::string const& error)
    : packet(ERROR), err(new char[error.size() + 1]), err_size(error.size()) {
        std::strcpy(this->err, error.c_str());
    }

    error_packet(net_interface& iface)
    : error_packet() {
        iface.receive(&this->err_size, sizeof(uint32_t));
        this->err = new char[this->err_size];
        iface.receive(this->err, this->err_size);
    }

    error_packet()
    : packet(ERROR), err(nullptr) {}

    ~error_packet() {
        delete[] err;
    }

    error_packet(error_packet& other) = delete;

    virtual void* serialise(size_t& size) const {
        size = sizeof(uint32_t) + err_size + sizeof(this->p_type);

        unsigned char* buf = (unsigned char*)malloc(size);
        size_t offset = 0;
        memcpy(buf + offset, &this->p_type, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, &this->err_size, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buf + offset, this->err, this->err_size);
        return buf;
    }
};
