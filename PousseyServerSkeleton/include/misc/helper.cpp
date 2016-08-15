#include "helper.hpp"
#include <cstdio>

namespace PousseyServer
{
	namespace Helper
	{
		std::pair<std::string, std::string> KeyValuePair( std::string const & str, char const delimeter )
		{
			std::string::const_iterator cpos = std::find( std::cbegin( str ), std::cend( str ), delimeter );
			if ( cpos != std::cend( str ) ){
				std::string const key{ std::string{ std::cbegin( str ), cpos } };
				std::string const value{ cpos + 1, std::cend( str ) };
				return{ key, value };
			}
			return{ "", str };
		}

		void LogBoostErrorThenExit( boost::system::error_code const & error )
		{
			int const error_value = error.value();
			std::fprintf( stderr, "Error: %d: %s", error_value, error.message().c_str() );
			std::exit( error_value );
		}

		void LogErrorThenExit( std::string const & error )
		{
			std::fprintf( stderr, error.c_str() );
			std::exit( -1 );
		}
	}

	namespace http
	{
		void request_t::ReadRemoteEndpointData( boost::asio::ip::tcp::socket & socket )
		{
			try {
				this->remoteEndpointIP = socket.lowest_layer().remote_endpoint();
			}
			catch ( std::exception const & ){
				// do nothing
			}
		}
	}
}
