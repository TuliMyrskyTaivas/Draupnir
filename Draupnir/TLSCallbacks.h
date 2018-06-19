////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	draupnir/TLSCallbacks.h
//
// summary:	Declares the TLS callbacks class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <botan/tls_callbacks.h>
#include <vector>

namespace Draupnir
{
	class TLSCallbacks : public Botan::TLS::Callbacks
	{
		int m_socket = -1;
		
		void RawWrite(const std::vector<uint8_t>&& buf);			

	public:
		explicit TLSCallbacks(int sock);

		void tls_emit_data(const uint8_t data[], size_t size) override;
		void tls_record_received(uint64_t seqNo, const uint8_t data[], size_t size) override;
		void tls_alert(Botan::TLS::Alert alert) override;
		bool tls_session_established(const Botan::TLS::Session& session) override;
		void tls_verify_cert_chain(const std::vector<Botan::X509_Certificate>& certChain,
			const std::vector<std::shared_ptr<const Botan::OCSP::Response>>& ocspResponses,
			const std::vector<Botan::Certificate_Store*>& trustedRoots,
			Botan::Usage_Type usage,
			const std::string& hostname,
			const Botan::TLS::Policy& policy) override;
	};
} // namespace Draupnir
