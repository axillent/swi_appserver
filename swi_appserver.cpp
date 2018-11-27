//============================================================================
// Name        : swi_appserver.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================
#define _SMARTLETSP_COMMON_JSON_EXT_CPP
#define _SMARTLETSP_COMMON_ROUTETABLE_FILE_CPP
#define _STAVRP_LINUX_LOG4CPP_CPP

#include <unistd.h>
#include <time.h>

#include <stavrp/platform/linux/log4cpp.hpp>
#include <stavrp/platform/linux/drv/tcp.hpp>
#include <stavrp/platform/linux/drv/mqtt.hpp>

#include <smartletsp/interface/transl/transl_inet.hpp>
#include <smartletsp/interface/transl/transl_mqtt.hpp>
#include <smartletsp/interface/in_p2p.hpp>
#include <smartletsp/node/node_basic.hpp>
#include <smartletsp/common/comm_routetable_file.hpp>
#include <smartletsp/msg/msg_appl.hpp>
#include <smartletsp/smartletsp.hpp>

#include "swi_appconfig.hpp"
#include "swi_appserver.hpp"

#include <smartletsp/common/comm_json.hpp>

//-----------------------------------------------------------------------------------------
// Applications class definition
//-----------------------------------------------------------------------------------------
template <class Node>
class ApplInet : public Smartlets::Node::Appl::Standard<Node> {
protected:
public:
	static void Loop(void);
	static Smartlets::Node::Appl::AcceptState AcceptRX(const Smartlets::Message::MessageBase& msg);
	static Smartlets::Node::Appl::AcceptState AcceptTX(const Smartlets::Message::MessageBase& msg);
};
template <class Node>
class ApplMqtt : public Smartlets::Node::Appl::Standard<Node> {
public:
	static Smartlets::Node::Appl::AcceptState AcceptRX(const Smartlets::Message::MessageBase& msg);
	static Smartlets::Node::Appl::AcceptState AcceptTX(const Smartlets::Message::MessageBase& msg);
};

//-----------------------------------------------------------------------------------------
// Nodes
//-----------------------------------------------------------------------------------------
// --- inet
typedef Smartlets::Common::RouteTableFile														RouteTable;
typedef STAVRP::Linux::TCPDrv<0>																TCPDrv;
typedef Smartlets::Interface::Translator::INet<Config::Defaults::Message, TCPDrv, RouteTable>	TranslINet;
typedef Smartlets::Interface::P2P<Config::Defaults::Message, TranslINet, 16, 16>				InterfaceINet;
typedef Smartlets::Node::NodeBasic<Config, InterfaceINet, ApplInet>								NodeINet;

// --- mqtt
typedef Smartlets::Interface::Translator::Mqtt<Config::Defaults::Message>				TranslMqtt;
typedef Smartlets::Interface::P2P<Config::Defaults::Message, TranslMqtt, 16, 16>		InterfaceMqtt;
typedef Smartlets::Node::NodeBasic<Config, InterfaceMqtt, ApplMqtt>						NodeMqtt;


//-----------------------------------------------------------------------------------------
// Main programm
//-----------------------------------------------------------------------------------------
int main(void) {

	Logger::Init();
	Logger::root() << log4cpp::Priority::DEBUG << "started";

	Smartlets::AppServer::AppConfig::Init("swi_appserver.properties");

	//---------------------------------------------------------------------------
	// starting inet
	//---------------------------------------------------------------------------
	TCPDrv::setPort(Smartlets::AppServer::AppConfig::getData().tcp_port);
	NodeINet::Init(1);
	TranslINet::GetRouteTable().SetFile(Smartlets::AppServer::AppConfig::getData().route_table_file);

	TranslMqtt::SetIPPortDomain(Smartlets::AppServer::AppConfig::getData().mqtt_server, Smartlets::AppServer::AppConfig::getData().mqtt_port, Smartlets::AppServer::AppConfig::getData().swi_domain);
	NodeMqtt::Init(1);

	while(1) {

		//---------------------------------------------------------------------------
		// loop inet
		//---------------------------------------------------------------------------
		if( NodeINet::Loop() ) {

			Config::Defaults::Message msg = NodeINet::RX();

			Logger::root() << log4cpp::Priority::DEBUG << "inet-> tx=" << static_cast<uint16_t>(msg.header.tx);

			NodeINet::CommitRX(false);

			// --- routing
			if( NodeINet::TX(msg) ) {
				Logger::root() << log4cpp::Priority::DEBUG << "inet->inet TX OK";
				// delayed queue
			} else {
				Logger::root() << log4cpp::Priority::ERROR << "inet->inet TX failed";
			}

			if( NodeMqtt::TX(msg) ) {
				Logger::root() << log4cpp::Priority::DEBUG << "inet->mqtt TX OK";
			} else {
				Logger::root() << log4cpp::Priority::ERROR << "inet->mqtt TX failed";
			}

		}

		//---------------------------------------------------------------------------
		// loop mqtt
		//---------------------------------------------------------------------------
		if( NodeMqtt::Loop() ) {

			Config::Defaults::Message msg = NodeMqtt::RX();

			Logger::root() << log4cpp::Priority::DEBUG << "mqtt-> tx=" << static_cast<uint16_t>(msg.header.tx);

			char buffer[Smartlets::Common::JSON<Config::Defaults::Message>::neededBufferSize];
			Smartlets::Common::JSON<Config::Defaults::Message>::MessageToJSON(buffer, sizeof(buffer), msg);
			Logger::root() << log4cpp::Priority::DEBUG << "mqtt json '" << buffer << "'";

			NodeMqtt::CommitRX(false);

			if( NodeINet::TX(msg) ) Logger::root() << log4cpp::Priority::DEBUG << "mqtt->inet TX OK";
			else Logger::root() << log4cpp::Priority::ERROR << "mqtt->inet TX failed";

		}

		usleep(100);

	}

}

//-----------------------------------------------------------------------------------------
// Methods definition Inet
//-----------------------------------------------------------------------------------------
template <class Node>
void ApplInet<Node>::Loop() {
//	TranslINet::GetRouteTable().Loop();
}
template <class Node>
Smartlets::Node::Appl::AcceptState ApplInet<Node>::AcceptRX(const Smartlets::Message::MessageBase& msg) {

	Logger::root() << log4cpp::Priority::DEBUG << "ApplInet::AceeptRX tx=" << static_cast<uint16_t>(msg.header.tx);

	if( Smartlets::Node::Appl::Standard<Node>::AcceptRX(msg) == Smartlets::Node::Appl::AcceptCommit ) {
		Logger::root() << log4cpp::Priority::DEBUG << "ApplInet::AceeptRX == AcceptCommit";
		return  Smartlets::Node::Appl::AcceptCommit;
	}

	if( msg.header.tx.IsBroadcast() ) {

		switch(msg.header.type) {
		case Smartlets::Message::Type::RouteAnnounce:
		{
			auto route = (Smartlets::Message::RouteAnnounce&)msg;

			TranslINet::GetRouteTable().addRoute(route.ip, route.port, route.segment, route.segments);

			Logger::root() << log4cpp::Priority::DEBUG << "add router segment=" << route.segment;
			return Smartlets::Node::Appl::AcceptCommit;
		}
		break;
		case Smartlets::Message::Type::LocalTimeRequest:
		{
			time_t t = time(NULL);
			struct tm* lt = localtime(&t);
			auto data = Smartlets::Message::LocalTime(lt->tm_year, lt->tm_mon+1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
			data.header.tx = msg.header.rx;

			Node::TX(data);
		}
		break;
		}

	}

	return Smartlets::Node::Appl::AcceptProceed;
}
template <class Node>
Smartlets::Node::Appl::AcceptState ApplInet<Node>::AcceptTX(const Smartlets::Message::MessageBase& msg) {

	Logger::root() << log4cpp::Priority::DEBUG << "ApplInet::AcceptTX tx=" << static_cast<uint16_t>(msg.header.tx);

	Smartlets::Node::Appl::Standard<Node>::AcceptTX(msg);

	return Smartlets::Node::Appl::AcceptProceed;
}
//-----------------------------------------------------------------------------------------
// Methods definition Mqtt
//-----------------------------------------------------------------------------------------
template <class Node>
Smartlets::Node::Appl::AcceptState ApplMqtt<Node>::AcceptRX(const Smartlets::Message::MessageBase& msg) {

	Logger::root() << log4cpp::Priority::DEBUG << "ApplMqtt::AceeptRX tx=" << static_cast<uint16_t>(msg.header.tx);

	if( Smartlets::Node::Appl::Standard<Node>::AcceptRX(msg) == Smartlets::Node::Appl::AcceptCommit ) {
		Logger::root() << log4cpp::Priority::DEBUG << "ApplMqtt::AceeptRX == AcceptCommit";
		return Smartlets::Node::Appl::AcceptCommit;
	}

	if( (msg.header.tx.IsBroadcastGlobal() && Smartlets::AppServer::AppConfig::getData().mqtt_broadcast_subscribe)
			|| (!msg.header.tx.IsBroadcastGlobal() && Smartlets::AppServer::AppConfig::getData().mqtt_message_subscribe) ) {

		Logger::root() << log4cpp::Priority::DEBUG << "ApplMqtt::AceeptRX == AcceptProceed";
		return Smartlets::Node::Appl::AcceptProceed;

	}

	Logger::root() << log4cpp::Priority::DEBUG << "ApplMqtt::AceeptRX == AcceptIgnore";
	return Smartlets::Node::Appl::AcceptSkip;
}
template <class Node>
Smartlets::Node::Appl::AcceptState ApplMqtt<Node>::AcceptTX(const Smartlets::Message::MessageBase& msg) {

	Logger::root() << log4cpp::Priority::DEBUG << "ApplMqtt::AceeptTX tx=" << static_cast<uint16_t>(msg.header.tx);

	Smartlets::Node::Appl::Standard<Node>::AcceptTX(msg);

	if( (msg.header.tx.IsBroadcastGlobal() && Smartlets::AppServer::AppConfig::getData().mqtt_broadcast_publish)
			|| ((!msg.header.tx.IsBroadcastGlobal()) && Smartlets::AppServer::AppConfig::getData().mqtt_message_publish) ) {

		return Smartlets::Node::Appl::AcceptProceed;

	}

	Logger::root() << log4cpp::Priority::DEBUG << "ApplMqtt::AceeptTX ignore " << Smartlets::AppServer::AppConfig::getData().mqtt_broadcast_publish << ", " << Smartlets::AppServer::AppConfig::getData().mqtt_message_publish;

	return Smartlets::Node::Appl::AcceptSkip;
}

