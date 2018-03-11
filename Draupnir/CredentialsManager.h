////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	draupnir/CredentialsManager.h
//
// summary:	Declares the credentials manager class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <botan/credentials_manager.h>
#include <memory>

namespace Draupnir
{
	class CredentialsManager final : public Botan::Credentials_Manager
	{
		Botan::X509_Certificate m_cert;
		std::unique_ptr<Botan::Private_Key> m_key;

	public:
		CredentialsManager();

		std::vector<Botan::Certificate_Store*> trusted_certificate_authorities(
			const std::string& type,
			const std::string& context) final override;
		std::vector<Botan::X509_Certificate> cert_chain(
			const std::vector<std::string>& certKeyTypes,
			const std::string& type,
			const std::string& context) final override;
		Botan::Private_Key* private_key_for(
			const Botan::X509_Certificate& cert,
			const std::string& type,
			const std::string& context) final override;
	};
} // namespace Draupnir
