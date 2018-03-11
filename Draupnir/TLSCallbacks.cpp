////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	draupnir/TLSCallbacks.cpp
//
// summary:	Implements the TLS callbacks class
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TLSCallbacks.h"
#include "Logger.h"

#include <botan/tls_policy.h>
#include <botan/x509path.h>
#include <botan/ocsp.h>

#include <stdexcept>
#include <cassert>
#include <cerrno>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

namespace Draupnir
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
	TLSCallbacks::TLSCallbacks(int sock)
		: m_socket(sock)
	{
		assert(-1 != m_socket && "invalid socket in TLS callback handler");
		const int flags = fcntl(m_socket, F_GETFL, 0);
		if (!(flags & O_NONBLOCK))
			throw std::runtime_error("blocking socket is used in TLS");
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void TLSCallbacks::RawWrite(const std::vector<uint8_t>&& buf)
	{
		auto size = buf.size();
		auto pos = buf.data();
		while (size)
		{
			const ssize_t res = write(m_socket, pos, size);
			if (-1 == res && (errno != EINTR || errno != EAGAIN))
				throw std::runtime_error("failed to write: " + std::string(strerror(errno)));

			size -= static_cast<size_t>(res);
			pos += res;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void TLSCallbacks::tls_emit_data(const uint8_t data[], size_t size)
	{
		Logger::GetInstance().Debug() << "TLS emit data: " << size << " bytes";
		RawWrite(std::vector<uint8_t>(data, data + size));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void TLSCallbacks::tls_record_received(uint64_t seqNo, const uint8_t data[], size_t size)
	{
		Logger::GetInstance().Debug() << "TLS record received: " << size << " bytes";
		RawWrite(std::vector<uint8_t>(data, data + size));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void TLSCallbacks::tls_alert(Botan::TLS::Alert alert)
	{
		if (Botan::TLS::Alert::CLOSE_NOTIFY == alert.type())
		{
			Logger::GetInstance().Debug() << "TLS close notitification received, closing the socket";
			close(m_socket);
		}
		else
		{
			Logger::GetInstance().Error() << "TLS alert: " << alert.type_string();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	bool TLSCallbacks::tls_session_established(const Botan::TLS::Session& session)
	{
		const auto& info = session.server_info();
		Logger::GetInstance().Debug() << "TLS session with " << info.hostname()
		    << ":" << info.port() << " established";
		Logger::GetInstance().Debug() << session.version().to_string() << " using "
			<< session.ciphersuite().to_string();
		return true; // enable caching of the session in the configured session manager
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void TLSCallbacks::tls_verify_cert_chain(
		const std::vector<Botan::X509_Certificate>& certChain,
		const std::vector<std::shared_ptr<const Botan::OCSP::Response>>& ocspResponses,
		const std::vector<Botan::Certificate_Store*>& trustedRoots,
		Botan::Usage_Type usage,
		const std::string& hostname,
		const Botan::TLS::Policy& policy)
	{
		if (certChain.empty())
			throw std::invalid_argument("certificate chain is empty");

		auto ocspTimeout = std::chrono::milliseconds(1000);
		Botan::Path_Validation_Restrictions restrictions(
			policy.require_cert_revocation_info(),
			policy.minimum_signature_strength());
		Botan::Path_Validation_Result result = Botan::x509_path_validate(
			certChain,
			restrictions,
			trustedRoots,
			hostname,
			usage,
			std::chrono::system_clock::now(),
			ocspTimeout,
			ocspResponses);
		Logger::GetInstance().Debug() << "certificate validation status: "
			<< result.result_string();
	}
} // namespace Draupnir
