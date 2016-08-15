#include "../include/server_config.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace PousseyServer
{
	server_config_t::server_config_t( server_settings_t settings_ ) :
		settings( settings_.configPath, settings_.checkingConfiguration, settings_.usingDaemon ),
		errorFlag( false )
	{
		InitializeServerConfig();
	}

	bool server_config_t::HasError() const { return errorFlag; }
	std::string server_config_t::Error() const { return error; }

	void server_config_t::InitializeServerConfig()
	{
		if ( !settings.configPath.empty() ){
			this->environmentVariables.insert( { "CONFIG_DIR", settings.configPath.string() } );
		}
		else {
			this->UseDefaultConfigOptions();
		}

		try {
			this->ReadConfigurationFile( settings.configPath.string() );
		}
		catch ( std::exception const & error ){
			this->error = error.what();
			this->errorFlag = true;
		}
	}

	void server_config_t::UseDefaultConfigOptions()
	{
		paths.serverConfigPath = settings.configPath = boost::filesystem::path{ POUSSEY_DIR"/config/" };
		paths.serverLogFilepath = boost::filesystem::path{ POUSSEY_DIR"/config/" };
		paths.serverErrorFilepath = boost::filesystem::path{ POUSSEY_DIR"/config/" };
		paths.serverExploitFilepath = boost::filesystem::path{ POUSSEY_DIR"/config/" };
		serverVersion = container::string{ POUSSEY_SERVER_VERSION };
	}

	void server_config_t::ReadConfigurationFile( std::string const & path )
	{
		std::string const configurationFilename{ "/server_config.json" };
		boost::filesystem::path fullConfigurationPath{ path + configurationFilename };
		if ( !boost::filesystem::exists( fullConfigurationPath ) ){
			throw std::exception{ "Configuration file/directory does not exist." };
		}

		boost::filesystem::ifstream file{ fullConfigurationPath };
		if ( !file ){
			throw std::exception{ "Unable to read the configuration file, make sure you have accessed privilege." };
		}

		this->ActivateServerRecord( file );
		// if we are here, all is well and good.
		if ( settings.checkingConfiguration ){
			PousseyServer::Helper::LogErrorThenExit( "Server configurations correctly verified." );
		}
	}

	void server_config_t::ActivateServerRecord( boost::filesystem::ifstream & file )
	{
		boost::property_tree::ptree ptree;
		try {
			boost::property_tree::json_parser::read_json( file, ptree );
		}
		catch ( boost::property_tree::json_parser_error const & parserError ){
			std::ostringstream ss;
			ss << "In " << parserError.filename() << ", line: " << parserError.line() << ", "
				<< parserError.what();
			BOOST_RETHROW std::exception{ ss.str().c_str() };
		}

		for ( auto const & keyValue : ptree )
		{
			auto key = keyValue.first;
			if ( key == std::string{ "Binding" } ){
				auto bindingValues = keyValue.second;
				for ( auto const & bindings : bindingValues )
				{
					server_binding_t serverBinding;
					serverBinding.bindingName = bindings.first;
					for ( auto & bindingKeyValues : bindings.second )
					{
						auto const key = bindingKeyValues.first;
						auto const value = bindingKeyValues.second.data();

						if ( key == std::string{ "Interface" } ){
							serverBinding.bindingIPAddress = value;
						}
						else if ( key == std::string{ "Port" } ) {
							serverBinding.bindingPort = static_cast< unsigned short >( std::stoul( value ) );
						}
						else if ( key == std::string{ "MaxKeepAlive" } ){
							serverBinding.bindingMaximumKeepAlive = std::stoul( value );
						}
						else if ( key == std::string{ "MaxRequestSize" } ){
							serverBinding.bindingMaximumRequestSize = std::stoul( value );
						}
						else if ( key == std::string{ "MaxUploadSize" } ){
							serverBinding.bindingMaximumUploadSize = std::stoull( value );
						}
						else {
							throw std::runtime_error{ "Invalid key used in binding parameter: " + key };
						}
					}
					configBindings.push_back( serverBinding );
				}
			}
			else if ( key == std::string{ "Hostname" } ){
				hostName = keyValue.second.data();
			}
			else if ( key == std::string{ "LogFile" } ){
				paths.serverLogFilepath = keyValue.second.data();
			}
			else if ( key == std::string{ "ErrorLog" } ){
				paths.serverErrorFilepath = keyValue.second.data();
			}
			else if ( key == std::string{ "DatabasePath" } ){
				paths.serverDatabaseFilepath = keyValue.second.data();
			}
			else {
				throw std::runtime_error{ "Invalid key found." };
			}
		}
		BOOST_ASSERT( !hostName.empty() );
	}

	server_t::server_t( server_settings_t settings, boost::asio::io_service &service ):
		serverConfigurationPtr(nullptr), 
		ioService( service )
	{
		serverConfigurationPtr.reset( new server_config_t{ settings } );
		if ( serverConfigurationPtr->HasError() ){
			PousseyServer::Helper::LogErrorThenExit( serverConfigurationPtr->Error() );
		}
		this->StartServer();
	}

	void server_t::StartServer()
	{
		std::fprintf( stderr, "Server started" );
		unsigned short defaultPort = DEFAULT_PORT;

		std::vector<boost::asio::ip::tcp::endpoint> hostEndpoints;
		auto & bindings = serverConfigurationPtr->configBindings;
		for ( unsigned int index = 0; index != bindings.size(); ++index ){
			if ( bindings[ index ].bindingIPAddress.empty() )
			{
				bindings[ index ].bindingIPAddress = serverConfigurationPtr->hostName;
			}
			if ( 0 == bindings[ index ].bindingPort )
			{
				bindings[ index ].bindingPort = defaultPort++;
			}
			hostEndpoints.emplace_back( boost::asio::ip::address::from_string( bindings[ index ].bindingIPAddress ),
				bindings[ index ].bindingPort );
		}

		try {
			auto acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>( ioService, hostEndpoints[0] );
			acceptor->set_option( boost::asio::socket_base::reuse_address( true ) );
			std::fprintf( stderr, "%s, %d\n", acceptor->local_endpoint().address().to_string().c_str(),
				acceptor->local_endpoint().port() );
			this->StartAcceptingIncomingConnection( acceptor );
			ioService.run();
		}
		catch ( boost::system::system_error const & errorCode )
		{
			Helper::LogErrorThenExit( errorCode.what() );
		}
	}

	void server_t::StartAcceptingIncomingConnection( std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor )
	{
		auto httpSocket = std::make_shared<boost::asio::ip::tcp::socket>( ioService );
		auto functor = [this, httpSocket, acceptor] ( boost::system::error_code const & errorCode )
		{
			this->StartAcceptingIncomingConnection( acceptor );
			if ( !errorCode )
			{
				boost::asio::ip::tcp::no_delay option( true );
				httpSocket->set_option( option );
				this->StartReadingRequests( httpSocket );
			}
		};
		acceptor->async_accept( *httpSocket, functor );
	}

	void server_t::StartReadingRequests( std::shared_ptr<boost::asio::ip::tcp::socket> socket )
	{
		auto request = std::make_shared<http::request_t>();
		request->ReadRemoteEndpointData( *socket );

		boost::asio::async_read_until( *socket, request->content.stream_buffer, "\r\n\r\n",
			boost::bind( &server_t::OnReadCompleted, this, _1, _2, socket, request ) );
	}

	// ToDo
	void server_t::OnReadCompleted( boost::system::error_code const & errorCode,
		size_t bytesTransferred, server_t::http_socket_ptr_t socket, std::shared_ptr<http::request_t> request )
	{

	}
}