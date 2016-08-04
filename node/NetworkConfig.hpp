/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2016  ZeroTier, Inc.  https://www.zerotier.com/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ZT_NETWORKCONFIG_HPP
#define ZT_NETWORKCONFIG_HPP

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <stdexcept>
#include <algorithm>

#include "../include/ZeroTierOne.h"

#include "Constants.hpp"
#include "Buffer.hpp"
#include "InetAddress.hpp"
#include "MulticastGroup.hpp"
#include "Address.hpp"
#include "CertificateOfMembership.hpp"
#include "Capability.hpp"
#include "Tag.hpp"
#include "Dictionary.hpp"

/**
 * Flag: allow passive bridging (experimental)
 */
#define ZT_NETWORKCONFIG_FLAG_ALLOW_PASSIVE_BRIDGING 0x0000000000000001ULL

/**
 * Flag: enable broadcast
 */
#define ZT_NETWORKCONFIG_FLAG_ENABLE_BROADCAST 0x0000000000000002ULL

/**
 * Flag: enable IPv6 NDP emulation for certain V6 address patterns
 */
#define ZT_NETWORKCONFIG_FLAG_ENABLE_IPV6_NDP_EMULATION 0x0000000000000004ULL

/**
 * Device is an active bridge
 */
#define ZT_NETWORKCONFIG_SPECIALIST_TYPE_ACTIVE_BRIDGE 0x0000020000000000ULL

/**
 * An anchor is a device that is willing to be one and has been online/stable for a long time on this network
 */
#define ZT_NETWORKCONFIG_SPECIALIST_TYPE_ANCHOR 0x0000040000000000ULL

namespace ZeroTier {

// Maximum size of a network config dictionary (can be increased)
#define ZT_NETWORKCONFIG_DICT_CAPACITY 8194

// Network config version
#define ZT_NETWORKCONFIG_VERSION 6

// Fields for meta-data sent with network config requests
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_VERSION "v"
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_PROTOCOL_VERSION "pv"
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_NODE_MAJOR_VERSION "majv"
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_NODE_MINOR_VERSION "minv"
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_NODE_REVISION "revv"
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_MAX_NETWORK_RULES "Mr"
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_MAX_CAPABILITY_RULES "Mcr"

// These dictionary keys are short so they don't take up much room.

// network config version
#define ZT_NETWORKCONFIG_DICT_KEY_VERSION "v"
// network ID
#define ZT_NETWORKCONFIG_DICT_KEY_NETWORK_ID "nwid"
// integer(hex)
#define ZT_NETWORKCONFIG_DICT_KEY_TIMESTAMP "ts"
// integer(hex)
#define ZT_NETWORKCONFIG_DICT_KEY_REVISION "r"
// address of member
#define ZT_NETWORKCONFIG_DICT_KEY_ISSUED_TO "id"
// flags(hex)
#define ZT_NETWORKCONFIG_DICT_KEY_FLAGS "f"
// integer(hex)
#define ZT_NETWORKCONFIG_DICT_KEY_MULTICAST_LIMIT "ml"
// network type (hex)
#define ZT_NETWORKCONFIG_DICT_KEY_TYPE "t"
// text
#define ZT_NETWORKCONFIG_DICT_KEY_NAME "n"
// binary serialized certificate of membership
#define ZT_NETWORKCONFIG_DICT_KEY_COM "C"
// specialists (binary array of uint64_t)
#define ZT_NETWORKCONFIG_DICT_KEY_SPECIALISTS "S"
// routes (binary blob)
#define ZT_NETWORKCONFIG_DICT_KEY_ROUTES "RT"
// static IPs (binary blob)
#define ZT_NETWORKCONFIG_DICT_KEY_STATIC_IPS "I"
// rules (binary blob)
#define ZT_NETWORKCONFIG_DICT_KEY_RULES "R"

// Legacy fields -- these are obsoleted but are included when older clients query

// boolean (now a flag)
#define ZT_NETWORKCONFIG_DICT_KEY_ALLOW_PASSIVE_BRIDGING_OLD "pb"
// boolean (now a flag)
#define ZT_NETWORKCONFIG_DICT_KEY_ENABLE_BROADCAST_OLD "eb"
// IP/bits[,IP/bits,...]
// Note that IPs that end in all zeroes are routes with no assignment in them.
#define ZT_NETWORKCONFIG_DICT_KEY_IPV4_STATIC_OLD "v4s"
// IP/bits[,IP/bits,...]
// Note that IPs that end in all zeroes are routes with no assignment in them.
#define ZT_NETWORKCONFIG_DICT_KEY_IPV6_STATIC_OLD "v6s"
// 0/1
#define ZT_NETWORKCONFIG_DICT_KEY_PRIVATE_OLD "p"
// integer(hex)[,integer(hex),...]
#define ZT_NETWORKCONFIG_DICT_KEY_ALLOWED_ETHERNET_TYPES_OLD "et"
// string-serialized CertificateOfMembership
#define ZT_NETWORKCONFIG_DICT_KEY_CERTIFICATE_OF_MEMBERSHIP_OLD "com"
// node[,node,...]
#define ZT_NETWORKCONFIG_DICT_KEY_ACTIVE_BRIDGES_OLD "ab"
// node;IP/port[,node;IP/port]
#define ZT_NETWORKCONFIG_DICT_KEY_RELAYS_OLD "rl"

/**
 * Network configuration received from network controller nodes
 *
 * This is a memcpy()'able structure and is safe (in a crash sense) to modify
 * without locks.
 */
class NetworkConfig
{
public:
	/**
	 * Create an instance of a NetworkConfig for the test network ID
	 *
	 * The test network ID is defined as ZT_TEST_NETWORK_ID. This is a
	 * "fake" network with no real controller and default options.
	 *
	 * @param self This node's ZT address
	 * @return Configuration for test network ID
	 */
	static inline NetworkConfig createTestNetworkConfig(const Address &self)
	{
		NetworkConfig nc;

		nc.networkId = ZT_TEST_NETWORK_ID;
		nc.timestamp = 1;
		nc.revision = 1;
		nc.issuedTo = self;
		nc.multicastLimit = ZT_MULTICAST_DEFAULT_LIMIT;
		nc.flags = ZT_NETWORKCONFIG_FLAG_ENABLE_BROADCAST;
		nc.type = ZT_NETWORK_TYPE_PUBLIC;

		nc.rules[0].t = ZT_NETWORK_RULE_ACTION_ACCEPT;
		nc.ruleCount = 1;

		Utils::snprintf(nc.name,sizeof(nc.name),"ZT_TEST_NETWORK");

		// Make up a V4 IP from 'self' in the 10.0.0.0/8 range -- no
		// guarantee of uniqueness but collisions are unlikely.
		uint32_t ip = (uint32_t)((self.toInt() & 0x00ffffff) | 0x0a000000); // 10.x.x.x
		if ((ip & 0x000000ff) == 0x000000ff) ip ^= 0x00000001; // but not ending in .255
		if ((ip & 0x000000ff) == 0x00000000) ip ^= 0x00000001; // or .0
		nc.staticIps[0] = InetAddress(Utils::hton(ip),8);

		// Assign an RFC4193-compliant IPv6 address -- will never collide
		nc.staticIps[1] = InetAddress::makeIpv6rfc4193(ZT_TEST_NETWORK_ID,self.toInt());

		nc.staticIpCount = 2;

		return nc;
	}

	NetworkConfig()
	{
		memset(this,0,sizeof(NetworkConfig));
	}

	NetworkConfig(const NetworkConfig &nc)
	{
		memcpy(this,&nc,sizeof(NetworkConfig));
	}

	inline NetworkConfig &operator=(const NetworkConfig &nc)
	{
		memcpy(this,&nc,sizeof(NetworkConfig));
		return *this;
	}

	/**
	 * Write this network config to a dictionary for transport
	 *
	 * @param d Dictionary
	 * @param includeLegacy If true, include legacy fields for old node versions
	 * @return True if dictionary was successfully created, false if e.g. overflow
	 */
	bool toDictionary(Dictionary<ZT_NETWORKCONFIG_DICT_CAPACITY> &d,bool includeLegacy) const;

	/**
	 * Read this network config from a dictionary
	 *
	 * @param d Dictionary
	 * @return True if dictionary was valid and network config successfully initialized
	 */
	bool fromDictionary(const Dictionary<ZT_NETWORKCONFIG_DICT_CAPACITY> &d);

	/**
	 * @return True if passive bridging is allowed (experimental)
	 */
	inline bool allowPassiveBridging() const throw() { return ((this->flags & ZT_NETWORKCONFIG_FLAG_ALLOW_PASSIVE_BRIDGING) != 0); }

	/**
	 * @return True if broadcast (ff:ff:ff:ff:ff:ff) address should work on this network
	 */
	inline bool enableBroadcast() const throw() { return ((this->flags & ZT_NETWORKCONFIG_FLAG_ENABLE_BROADCAST) != 0); }

	/**
	 * @return True if IPv6 NDP emulation should be allowed for certain "magic" IPv6 address patterns
	 */
	inline bool ndpEmulation() const throw() { return ((this->flags & ZT_NETWORKCONFIG_FLAG_ENABLE_IPV6_NDP_EMULATION) != 0); }

	/**
	 * @return Network type is public (no access control)
	 */
	inline bool isPublic() const throw() { return (this->type == ZT_NETWORK_TYPE_PUBLIC); }

	/**
	 * @return Network type is private (certificate access control)
	 */
	inline bool isPrivate() const throw() { return (this->type == ZT_NETWORK_TYPE_PRIVATE); }

	/**
	 * @return ZeroTier addresses of devices on this network designated as active bridges
	 */
	inline std::vector<Address> activeBridges() const
	{
		std::vector<Address> r;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_ACTIVE_BRIDGE) != 0)
				r.push_back(Address(specialists[i]));
		}
		return r;
	}

	/**
	 * @return ZeroTier addresses of "anchor" devices on this network
	 */
	inline std::vector<Address> anchors() const
	{
		std::vector<Address> r;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_ANCHOR) != 0)
				r.push_back(Address(specialists[i]));
		}
		return r;
	}

	/**
	 * @param fromPeer Peer attempting to bridge other Ethernet peers onto network
	 * @return True if this network allows bridging
	 */
	inline bool permitsBridging(const Address &fromPeer) const
	{
		if ((flags & ZT_NETWORKCONFIG_FLAG_ALLOW_PASSIVE_BRIDGING) != 0)
			return true;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((fromPeer == specialists[i])&&((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_ACTIVE_BRIDGE) != 0))
				return true;
		}
		return false;
	}

	/**
	 * @return True if this network config is non-NULL
	 */
	inline operator bool() const throw() { return (networkId != 0); }

	inline bool operator==(const NetworkConfig &nc) const { return (memcmp(this,&nc,sizeof(NetworkConfig)) == 0); }
	inline bool operator!=(const NetworkConfig &nc) const { return (!(*this == nc)); }

	/**
	 * Add a specialist or mask flags if already present
	 *
	 * This masks the existing flags if the specialist is already here or adds
	 * it otherwise.
	 *
	 * @param a Address of specialist
	 * @param f Flags (OR of specialist role/type flags)
	 * @return True if successfully masked or added
	 */
	inline bool addSpecialist(const Address &a,const uint64_t f)
	{
		const uint64_t aint = a.toInt();
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & 0xffffffffffULL) == aint) {
				specialists[i] |= f;
				return true;
			}
		}
		if (specialistCount < ZT_MAX_NETWORK_SPECIALISTS) {
			specialists[specialistCount++] = f | aint;
			return true;
		}
		return false;
	}

	const Capability *capability(const uint32_t id) const
	{
		for(unsigned int i=0;i<capabilityCount;++i) {
			if (capabilities[i].id() == id)
				return &(capabilities[i]);
		}
		return (Capability *)0;
	}

	const Tag *tag(const uint32_t id) const
	{
		for(unsigned int i=0;i<tagCount;++i) {
			if (tags[i].id() == id)
				return &(tags[i]);
		}
		return (Tag *)0;
	}

	/*
	inline void dump() const
	{
		printf("networkId==%.16llx\n",networkId);
		printf("timestamp==%llu\n",timestamp);
		printf("revision==%llu\n",revision);
		printf("issuedTo==%.10llx\n",issuedTo.toInt());
		printf("multicastLimit==%u\n",multicastLimit);
		printf("flags=%.8lx\n",(unsigned long)flags);
		printf("specialistCount==%u\n",specialistCount);
		for(unsigned int i=0;i<specialistCount;++i)
			printf("  specialists[%u]==%.16llx\n",i,specialists[i]);
		printf("routeCount==%u\n",routeCount);
		for(unsigned int i=0;i<routeCount;++i) {
			printf("  routes[i].target==%s\n",reinterpret_cast<const InetAddress *>(&(routes[i].target))->toString().c_str());
			printf("  routes[i].via==%s\n",reinterpret_cast<const InetAddress *>(&(routes[i].via))->toIpString().c_str());
			printf("  routes[i].flags==%.4x\n",(unsigned int)routes[i].flags);
			printf("  routes[i].metric==%u\n",(unsigned int)routes[i].metric);
		}
		printf("staticIpCount==%u\n",staticIpCount);
		for(unsigned int i=0;i<staticIpCount;++i)
			printf("  staticIps[i]==%s\n",staticIps[i].toString().c_str());
		printf("ruleCount==%u\n",ruleCount);
		printf("name==%s\n",name);
		printf("com==%s\n",com.toString().c_str());
	}
	*/

	/**
	 * Network ID that this configuration applies to
	 */
	uint64_t networkId;

	/**
	 * Controller-side time of config generation/issue
	 */
	uint64_t timestamp;

	/**
	 * Controller-side revision counter for this configuration
	 */
	uint64_t revision;

	/**
	 * Address of device to which this config is issued
	 */
	Address issuedTo;

	/**
	 * Flags (64-bit)
	 */
	uint64_t flags;

	/**
	 * Maximum number of recipients per multicast (not including active bridges)
	 */
	unsigned int multicastLimit;

	/**
	 * Number of specialists
	 */
	unsigned int specialistCount;

	/**
	 * Number of routes
	 */
	unsigned int routeCount;

	/**
	 * Number of ZT-managed static IP assignments
	 */
	unsigned int staticIpCount;

	/**
	 * Number of pinned devices (devices with physical address hints)
	 */
	unsigned int pinnedCount;

	/**
	 * Number of rule table entries
	 */
	unsigned int ruleCount;

	/**
	 * Number of capabilities
	 */
	unsigned int capabilityCount;

	/**
	 * Number of tags
	 */
	unsigned int tagCount;

	/**
	 * Specialist devices
	 *
	 * For each entry the least significant 40 bits are the device's ZeroTier
	 * address and the most significant 24 bits are flags indicating its role.
	 */
	uint64_t specialists[ZT_MAX_NETWORK_SPECIALISTS];

	/**
	 * Statically defined "pushed" routes (including default gateways)
	 */
	ZT_VirtualNetworkRoute routes[ZT_MAX_NETWORK_ROUTES];

	/**
	 * Static IP assignments
	 */
	InetAddress staticIps[ZT_MAX_ZT_ASSIGNED_ADDRESSES];

	/**
	 * Base network rules
	 */
	ZT_VirtualNetworkRule rules[ZT_MAX_NETWORK_RULES];

	/**
	 * Capabilities for this node on this network
	 */
	Capability capabilities[ZT_MAX_NETWORK_CAPABILITIES];

	/**
	 * Tags for this node on this network
	 */
	Tag tags[ZT_MAX_NETWORK_TAGS];

	/**
	 * Network type (currently just public or private)
	 */
	ZT_VirtualNetworkType type;

	/**
	 * Network short name or empty string if not defined
	 */
	char name[ZT_MAX_NETWORK_SHORT_NAME_LENGTH + 1];

	/**
	 * Certficiate of membership (for private networks)
	 */
	CertificateOfMembership com;
};

} // namespace ZeroTier

#endif
