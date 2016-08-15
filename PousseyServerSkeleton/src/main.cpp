#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <tclap/CmdLine.h>
#include "../include/server_config.hpp"

int main( int argc, char **argv )
{
	char const *version_number = "1.0";
	TCLAP::CmdLine command_line{ "Command Line Parameters", ' ', version_number };
	PousseyServer::server_settings_t settings { "", false, true };

	try {
		TCLAP::ValueArg<std::string> config_path{ "p", "path", "Path to the configuration", false, "", 
			"Path to the directory holding the configuration files."
		};
		TCLAP::SwitchArg check_configuration_entry{ "c", "check-config", "simply check configuration and exit", false };
		TCLAP::SwitchArg use_daemon_entry{ "d", "use-daemon", "Start the server as a background daemon", true };

		command_line.add( config_path );
		command_line.add( use_daemon_entry );
		command_line.add( check_configuration_entry );
		command_line.parse( argc, argv );

		settings.configPath = boost::filesystem::path{ config_path.getValue() };
		settings.checkingConfiguration = check_configuration_entry.getValue();
		settings.usingDaemon = use_daemon_entry.getValue();
	}
	catch ( TCLAP::ArgException const & exception ) {
		std::cerr << "Error: " << exception.error() << " for argument " << exception.argId()
			<< std::endl;
		return -1;
	}

	boost::asio::io_service io_service;
	auto server = std::make_shared<PousseyServer::server_t>( settings, io_service );
	server->StartServer();
	return 0;
}