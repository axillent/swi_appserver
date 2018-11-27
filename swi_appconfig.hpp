/*
 * swi_config.hpp
 *
 *  Created on: 28 сент. 2018 г.
 *      Author: axill
 */

#ifndef SWI_APPCONFIG_HPP_
#define SWI_APPCONFIG_HPP_

#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>

namespace Smartlets {
namespace AppServer {

class AppConfig {
public:
	struct Data {
		// --- httpd
		uint16_t			httpd_port;
		// --- tcp server
		uint16_t			tcp_port;
		// --- mqtt
		std::string			mqtt_server;
		uint16_t			mqtt_port;
		uint16_t			mqtt_keepalive;
		bool				mqtt_broadcast_publish;
		bool				mqtt_broadcast_subscribe;
		bool				mqtt_message_publish;
		bool				mqtt_message_subscribe;
		// --- swi
		std::string			swi_domain;
		// --- route
		std::string			route_table_file;
	};
protected:
	static Data		data;
public:
	static bool Init(const char* file);
	static Data& getData() { return data; }
	static void setData(Data& d);
};

}}

//---------------------------------------------------------------------------

#ifndef NOINSOURCE

namespace Smartlets {
namespace AppServer {

AppConfig::Data AppConfig::data;

bool AppConfig::Init(const char* file) {

	std::ifstream	in(file);
	std::string		str;
	size_t			n;

	Logger::root() << log4cpp::Priority::DEBUG << "reading '" << file << "'";

	while( std::getline(in, str)) {

		if( str.find_first_of("\n\r") != std::string::npos ) str.pop_back();

		if( str[0] == '#' ) continue;

		if( (n=str.find_first_of('=')) == std::string::npos ) continue;

		std::string key = str.substr(0, n);
		std::string value = str.substr(n+1, std::string::npos);

		Logger::root() << log4cpp::Priority::DEBUG << "key=" << key << " value=" << value;

		// --- tcp server
		if( key == "tcpserver.port" ) data.tcp_port = stoul(value);

		// --- httpd
		if( key == "httpd.port" ) data.httpd_port = stoul(value);

		// --- mqtt server
		if( key == "mqtt.server" ) data.mqtt_server = value;
		if( key == "mqtt.port" ) data.mqtt_port = stoul(value);
		if( key == "mqtt.keepalive" ) data.mqtt_keepalive = stoul(value);
		// --- mqtt <-> broadcast
		if( key == "mqtt.broadcast.publish" ) data.mqtt_broadcast_publish = (value=="true")?true:false;
		if( key == "mqtt.broadcast.subscribe" ) data.mqtt_broadcast_subscribe = (value=="true")?true:false;
		// --- mqtt <-> message
		if( key == "mqtt.message.publish" ) data.mqtt_message_publish = (value=="true")?true:false;
		if( key == "mqtt.message.subscribe" ) data.mqtt_message_subscribe = (value=="true")?true:false;

		// --- swi
		if( key == "swi.domain" ) data.swi_domain = value;

		// --- swi
		if( key == "route.table.file" ) data.route_table_file = value;

	}

	return true;
}


}}

#endif

#endif /* SWI_APPCONFIG_HPP_ */
