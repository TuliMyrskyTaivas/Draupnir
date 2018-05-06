////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Droupnir/ControlConductor.h
//
// summary:	Declares the control conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Posix.h"
#include "Conductor.h"
#include "TLSPolicy.h"
#include "CredentialsManager.h"

#include <botan/auto_rng.h>
#include <botan/tls_client.h>
#include <botan/tls_session_manager.h>

#include <memory>

namespace Draupnir
{
	class ControlConductor final : public Conductor, private Botan::TLS::Callbacks
	{
		SocketHandle m_socket;

		TLSPolicy m_policy;
		CredentialsManager m_creds;
		Botan::AutoSeeded_RNG m_rng;
		Botan::TLS::Session_Manager_In_Memory m_sessionMgr;
		Botan::TLS::Client m_tls;

		// Botan::TLS::Callbacks implementation
		void tls_record_received(uint64_t seqNo, const uint8_t data[], size_t size) final override;
		void tls_emit_data(const uint8_t data[], size_t size) final override;
		void tls_alert(Botan::TLS::Alert alert) final override;
		bool tls_session_established(const Botan::TLS::Session& session) final override;
		void tls_verify_cert_chain(const std::vector<Botan::X509_Certificate>& certChain,
			const std::vector<std::shared_ptr<const Botan::OCSP::Response>>& ocspResponses,
			const std::vector<Botan::Certificate_Store*>& trustedRoots,
			Botan::Usage_Type usage,
			const std::string& hostname,
			const Botan::TLS::Policy& policy) final override;

		SocketHandle ConnectSocket() const;
		void WriteToSocket(const std::vector<uint8_t>&& buf) const;
	public:
		virtual ~ControlConductor() = default;
		void Run() final;

	protected:
		friend class Conductor;
		ControlConductor(std::shared_ptr<Config> config);
	};
} // namespace Draupnir
