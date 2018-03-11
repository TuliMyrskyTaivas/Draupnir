////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/TLSPolicy.h
//
// summary:	Declares the TLS policy class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <botan/tls_policy.h>

namespace Draupnir
{
	class TLSPolicy : public Botan::TLS::Policy
	{
	public:
		std::vector<std::string> allowed_ciphers() const final override;
		std::vector<std::string> allowed_macs() const final override;
		std::vector<std::string> allowed_key_exchange_methods() const final override;
		std::vector<std::string> allowed_signature_hashes() const final override;
		std::vector<std::string> allowed_signature_methods() const final override;
		std::vector<std::string> allowed_ecc_curves() const final override;
		std::vector<uint8_t> compression() const final override;
		bool acceptable_protocol_version(Botan::TLS::Protocol_Version version) const final override;
		bool server_uses_own_ciphersuite_preferences() const final override;
	};
} // namespace Draupnir
