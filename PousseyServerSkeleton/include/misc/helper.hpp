#pragma once

#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <unordered_map>
#include <tuple>
#include <string>
#include <algorithm>

#ifdef _MSC_VER
#define FUNCTION_NO_RETURN __declspec( noreturn )
#else
#define FUNCTION_NO_RETURN [[noreturn]]
#endif

namespace PousseyServer
{
	namespace Helper
	{
		std::pair<std::string, std::string> KeyValuePair( std::string const & str, char const delimeter );
		FUNCTION_NO_RETURN void LogBoostErrorThenExit( boost::system::error_code const & errorMessage );
		FUNCTION_NO_RETURN void LogErrorThenExit( std::string const & errorMessage );
	}

	namespace http {
		enum class http_methods_t : uint16_t
		{
			HEAD = 0x0,
			GET,
			PUT,
			POST,
			CONNECT
		};

		// as defined by https://tools.ietf.org/html/rfc7230#section-3
		template<typename stream_type>
		struct message_body_t
		{
			boost::asio::streambuf	stream_buffer;
			stream_type				stream;

			message_body_t() : stream_buffer{}, stream{ &stream_buffer }
			{
			}

			size_t Size() const
			{
				return stream_buffer.size();
			}
		};

		struct header_t
		{
		private:
			struct equal_to_t
			{
				bool operator()( std::string const & key1, std::string const & key2 ) const
				{
					return boost::algorithm::iequals( key1, key2 );
				}
			};

			struct hash_t {
				size_t operator()( std::string const & key ) const
				{
					size_t seed = 0;
					for ( auto const & k : key ){
						boost::hash_combine( seed, std::tolower( k ) );
					}
					return seed;
				}
			};
		public:
			std::unordered_multimap<std::string, std::string, hash_t, equal_to_t> headerInfo;
		};

		using response_stream_t = message_body_t<std::ostream>;
		using request_stream_t =  message_body_t<std::istream>;

		struct request_t
		{
			std::string			httpMethod; // such as GET, POST, PUT, CONNECT etc
			std::string			requestTarget; // signifies the path
			std::string			httpVersion; // such as HTTP/1.1
			header_t			header;
			request_stream_t	content;

			// additional field to identify the remote endpoints( ipv4/ipv6 ), mainly used by the server
			boost::asio::ip::tcp::endpoint remoteEndpointIP;

			void ReadRemoteEndpointData( boost::asio::ip::tcp::socket & socket );
		};

		struct response_t
		{
			std::string			httpVersion;
			std::string			statusCode;
			std::string			reasonPhrase;
			header_t			header;
			response_stream_t	content;

			template<typename T>
			friend response_t & operator<<( response_t & response, T const & value )
			{
				return response.content.stream << value;
			}

			std::string String() const
			{
				std::ostringstream ss;
				ss << this->content.stream.rdbuf();
				return ss.str();
			}
		};
	}
}