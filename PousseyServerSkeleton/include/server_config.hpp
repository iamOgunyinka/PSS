#pragma once

#include <boost/asio.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/string.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <string>
#include <memory>
#include <cstdint>
#include "misc/helper.hpp"

#define POUSSEY_SERVER_VERSION "1.0"
#define DEFAULT_PORT 6028

#ifndef POUSSEY_DIR
#define POUSSEY_DIR "."
#endif

#define LOG_ERROR_EXIT(error) ()

namespace PousseyServer
{
	namespace container = std;

	struct server_settings_t
	{
		boost::filesystem::path	configPath;
		bool					checkingConfiguration;
		bool					usingDaemon;

		server_settings_t( boost::filesystem::path path, bool checkingConfig, bool useDaemon ) :
			configPath( path ), checkingConfiguration( checkingConfig ), usingDaemon( useDaemon )
		{
		}
	};
	
	struct server_binding_t
	{
		std::string			bindingName;
		std::string			bindingIPAddress;
		unsigned int		bindingMaximumKeepAlive;
		unsigned short		bindingPort = 0;
		uint64_t			bindingMaximumRequestSize = 128; // 128KB
		uint64_t			bindingMaximumUploadSize = 2048; // 2MB
		bool				bindingReuseAdddress = true;
	};

	struct server_directory_t
	{
		boost::filesystem::path		serverConfigPath;
		boost::filesystem::path		serverLogFilepath;
		boost::filesystem::path		serverErrorFilepath;
		boost::filesystem::path		serverExploitFilepath;
		boost::filesystem::path		serverDatabaseFilepath;
	};

	struct server_config_t
	{
		container::string					serverVersion;
		container::string					hostName;
		container::string					serverLogFile;
		std::vector<server_binding_t>		configBindings;
		std::vector<container::string>		supportedCGIExtensions;
		PousseyServer::server_directory_t	paths;
		
		server_config_t( server_settings_t settings_ );
		bool  HasError() const;
		std::string  Error() const;
	private:
		void InitializeServerConfig();
		void UseDefaultConfigOptions();
		void ReadConfigurationFile( std::string const & path );
		void ActivateServerRecord( boost::filesystem::ifstream & file );
	private:
		std::string							error;
		bool								errorFlag;
		server_settings_t					settings;
		std::map<std::string, std::string>	environmentVariables;
	};

	class server_t : public boost::noncopyable
	{
	public:
		using http_socket_ptr_t = std::shared_ptr<boost::asio::ip::tcp::socket>;
		server_t( server_settings_t settings, boost::asio::io_service & );
		void StartServer();
		void StopServer();
	private:
		void StartAcceptingIncomingConnection( std::shared_ptr<boost::asio::ip::tcp::acceptor> );
		void StartReadingRequests( http_socket_ptr_t socket );
		void OnReadCompleted( boost::system::error_code const &, size_t, http_socket_ptr_t, 
			std::shared_ptr<http::request_t> );
	private:
		std::shared_ptr<server_config_t>	serverConfigurationPtr;
		boost::asio::io_service	&			ioService;
	};
}