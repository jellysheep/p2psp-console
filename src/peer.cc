//
//  peer.cc -- Console version of a P2PSP peer
//
//  This code is distributed under the GNU General Public License (see
//  THE_GENERAL_GNU_PUBLIC_LICENSE.txt for extending this information).
//
//  Copyright (C) 2016, the P2PSP team.
//
//  http://www.p2psp.org
//
//  This program waits for the connection of the player, retrieves the
//  header from the source (and posiblely some stream), retrieves the
//  team configuration from the splitter and finally, feeds the player
//  with the chunks that gathers a peer.
//

// {{{ includes

#include <boost/format.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "common.h"
#include "core/common.h"
#if defined __IMS__
#include "core/peer_ims.h"
#elif defined __DBS__
#include "core/peer_dbs.h"
#endif
#include "util/trace.h"

// }}}

namespace p2psp {
  using namespace std;
  using namespace boost;
  
#if defined __IMS__  
  class Console: public Peer_IMS {
#elif defined __DBS__
  class Console: public Peer_DBS {
#endif
    // {{{

  protected:

    struct Source {
      ip::address addr;
      uint16_t port;
    };
    
    uint16_t player_port_;
    io_service io_service_;
    ip::tcp::acceptor acceptor_;
    ip::tcp::socket source_socket_;
    ip::tcp::socket player_socket_;
    int header_length_;
    struct Source source;
    std::string GET_message_;
    //std::string channel_;
    boost::array<char, 80> channel_;
    
  public:

    Console() : io_service_(),
	       acceptor_(io_service_),
	       source_socket_(io_service_),
	       player_socket_(io_service_) {
      //header_length_ = GetDefaultHeaderSize();
      //channel_ = GetDefaultChannel();
      //SetGETMessage(channel_);
      TRACE("Console initialized");
    }

    ~Console() {}

    void ReceiveChannel() {
      splitter_socket_.receive(boost::asio::buffer(channel_));
      TRACE("channel = " << std::to_string(channel_));
      SetGETMessage(channel_)
    }

    void ReceiveHeaderLength() {
      boost::array<char, 2> buffer;
      read(splitter_socket_, ::buffer(buffer));
      
      header_length_ = ntohs(*(short *)(buffer.c_array()));
      
      TRACE("header_length (in bytes) = "
	    << std::to_string(header_length_));
    }
  
    static std::string GetDefaultChannel() {
      return "BBB-143.ogv";
    }

    static int GetDefaultHeaderSize() {
      return 4096;
    }

    std::string GetChannel() {
      // {{{

      return channel_;
      
      // }}}
    }

    void ReceiveSourceEndpoint() {
      // {{{
      
      boost::array<char, 6> buffer;
      read(splitter_socket_, ::buffer(buffer));
      
      char *raw_data = buffer.data();
      
      in_addr ip_raw = *(in_addr *)(raw_data);
      source.addr = ip::address::from_string(inet_ntoa(ip_raw));
      source.port = ntohs(*(short *)(raw_data + 4));
      
      TRACE("source_endpoint = ("
	    << source.addr.to_string()
	    << ","
	    << std::to_string(source.port)
	    << ")");
      
      // }}}
    }

    void SetChannel(std::string channel) {
      // {{{

      channel_ = channel;
      SetGETMessage(channel_);

      // }}}
    }

    void SetSourceAddr(ip::address addr) {
      // {{{

      source.addr = addr;

      // }}}
    }

    ip::address GetSourceAddr() {
      // {{{
      
      return source.addr;

      // {{{
    }
    
    static ip::address GetDefaultSourceAddr() {
      // {{{

      return ip::address::from_string("127.0.0.1");

      // }}}
    }

    void SetSourcePort(uint16_t port) {
      // {{{

      source.port = port;

      // }}}
    }

    uint16_t GetSourcePort() {
      // {{{
      
      return source.port;

      // }}}
    }
    
    uint16_t GetHeaderLength() {
      // {{{
      
      return header_length_;

      // }}}
    }
    
    static uint16_t GetDefaultSourcePort() {
      // {{{

      return 8000;

      // }}}
    }

    void SetGETMessage(std::string channel) {
      std::stringstream ss;
      ss << "GET /" << channel << " HTTP/1.1\r\n"
	 << "\r\n";
      GET_message_ = ss.str();
      ss.str("");
    }

    void ConnectToTheSource() throw(boost::system::system_error) {
      // {{{

      ip::tcp::endpoint source_ep(this->source.addr, this->source.port);
      system::error_code ec;
      source_socket_.connect(source_ep, ec);

      if (ec) {
	ERROR(ec.message());
	ERROR(source_socket_.local_endpoint().address().to_string()
	    << "\b: unable to connect to the source ("
	    << source.addr
	    << ", "
	    << to_string(source.port)
	    << ")");
	source_socket_.close();
	exit(-1);
      }
      
      TRACE("Connected to the source at ("
	    << this->source.addr.to_string()
	    << ","
	    << std::to_string(this->source.port)
	    << ") from "
	    << source_socket_.local_endpoint().address().to_string());

      source_socket_.send(asio::buffer(GET_message_));

      TRACE("GET_message = "
	    << GET_message_);
      // }}}
    }
    
    void RelayHeader() {
      // {{{
            
      boost::array<char, 128> buf;
      //boost::system::error_code error;
      for(int header_load_counter_ = 0; header_load_counter_ < GetHeaderLength();) {

	//size_t len = socket.read_some(boost::asio::buffer(buf), error);
	size_t len = source_socket_.read_some(boost::asio::buffer(buf));
	header_load_counter_ += len;

	if (len <= 0) break;

	player_socket_.send(boost::asio::buffer(buf,len));
	//boost::asio::write(socket, boost::asio::buffer(message), ignored_error);
      }
      
      // }}}
    }
    
    void WaitForThePlayer() {
      // {{{
      
      //std::string port = std::to_string(player_port_);
      ip::tcp::endpoint endpoint(ip::tcp::v4(), player_port_);

      acceptor_.open(endpoint.protocol());
      acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
      acceptor_.bind(endpoint);
      acceptor_.listen();

      TRACE("Waiting for the player at ("
	    << endpoint.address().to_string()
	    << ","
            << std::to_string(endpoint.port())
            << ")");

      acceptor_.accept(player_socket_);

      TRACE("Player connected. Player is ("
            << player_socket_.remote_endpoint().address().to_string()
	    << ","
            << std::to_string(player_socket_.remote_endpoint().port())
	    << ")");

      // }}}
    }

    void SetHeaderSize(int header_length) {
      // {{{

      this->header_length_ = header_length;

      // }}}
    }

    int GetHeaderSize() {
      // {{{
      
      return this->header_length_;
      
      // }}}
    }

    bool PlayChunk(std::vector<char> chunk) {
      // {{{

      try {
        write(player_socket_, buffer(chunk));
        return true;
      } catch (std::exception e) {
        TRACE("Player disconnected!");
        //player_alive_ = false;
	return false;
      }

      // }}}
    }
    
    void SetPlayerPort(uint16_t player_port) {
      // {{{
      
      player_port_ = player_port;
      
      // }}}
    }

    uint16_t GetPlayerPort() {
      // {{{
      
      return  player_port_;
      
      // }}}
    }
      
    static uint16_t GetDefaultPlayerPort() {
      // {{{

      return 9999;

      // }}}
    }

    // }}}
  };

  int run(int argc, const char* argv[]) throw(boost::system::system_error) {

    // {{{ Argument Parsing

    const char description[80] = "This is the peer node of a P2PSP team.\n"
#if defined __IMS__
      "Using IMS.\n"
#elif defined __DBS__
      "Using DBS.\n"
#endif
      "Parameters";

    boost::program_options::options_description desc(description);

    {

      uint16_t player_port = Console::GetDefaultPlayerPort();
      //      std::string source_addr = Console::GetDefaultSourceAddr().to_string();
      //      uint16_t source_port = Console::GetDefaultSourcePort();
      std::string splitter_addr = p2psp::Peer_core::GetDefaultSplitterAddr().to_string();
      uint16_t splitter_port = p2psp::Peer_core::GetDefaultSplitterPort();
#if defined __DBS__
      int max_chunk_debt = p2psp::Peer_DBS::GetDefaultMaxChunkDebt();
      uint16_t team_port = p2psp::Peer_core::GetDefaultTeamPort();
#endif
#if defined __NTS__
      int source_port_step = 0;
#endif
      //      std::string channel = Console::GetDefaultChannel();
      //      int header_length = Console::GetDefaultHeaderSize();

      // TODO: strpe option should expect a list of arguments, not bool
      desc.add_options()
        ("help,h", "Produce this help message and exits.")
	//	("channel", boost::program_options::value<std::string>()->default_value(channel), "Name of the channel served by the streaming source.")
#if defined __DBS__
        ("enable_chunk_loss", boost::program_options::value<std::string>(), "Forces a lost of chunks.")
#endif
	//("header_length", boost::program_options::value<int>()->default_value(header_length), "Size of the header of the stream in chunks.")
#if defined __DBS__
        ("max_chunk_debt", boost::program_options::value<int>()->default_value(max_chunk_debt), "Maximum number of times that other peer can not send a chunk to this peer.")
#endif
        ("player_port", boost::program_options::value<uint16_t>()->default_value(player_port), "Port to communicate with the player.")
#if defined __NTS__
        ("source_port_step", boost::program_options::value<int>()->default_value(source_port_step), "Source port step forced when behind a sequentially port allocating NAT (conflicts with --chunk_loss_period).")
#endif
        //("source_addr", boost::program_options::value<std::string>()->default_value(source_addr), "IP address or hostname of the source.")
        //("source_port", boost::program_options::value<uint16_t>()->default_value(source_port), "Listening port of the source.")
        ("splitter_addr", boost::program_options::value<std::string>()->default_value(splitter_addr), "IP address or hostname of the splitter.")
        ("splitter_port", boost::program_options::value<uint16_t>()->default_value(splitter_port), "Listening port of the splitter.")
#if not defined __IMS__
        ("team_port", boost::program_options::value<uint16_t>()->default_value(team_port), "Port to communicate with the peers. By default the OS will chose it.")
        ("use_localhost", "Forces the peer to use localhost instead of the IP of the adapter to connect to the splitter." "Notice that in this case, peers that run outside of the host will not be able to communicate with this peer.")
#endif
        //"malicious",
        // boost::program_options::value<bool>()->implicit_value(true),
        //"Enables the malicious activity for peer.")(
        //("persistent",
        // boost::program_options::value<std::string>()->default_value(persistent),
        // "Forces the peer to send poisoned chunks to other peers.")
        //("on_off_ratio",
        // boost::program_options::value<int>()->default_value(on_off_ratio),
        // "Enables on-off attack and sets ratio for on off (from 1 to 100).")
        //("selective",
        // boost::program_options::value<std::string>()->default_value(selective),
        // "Enables selective attack for given set of peers.")
        //("bad_mouth",
        // boost::program_options::value<std::string>()->default_value(bad_mouth),
        // "Enables Bad Mouth attack for given set of peers.")
        // "trusted", boost::program_options::value<bool>()->implicit_value(true),
        // "Forces the peer to send hashes of chunks to splitter")(
        //("checkall",
        // "Forces the peer to send hashes of every chunks to splitter (works only with trusted option)")
        // "strpeds", boost::program_options::value<bool>()->implicit_value(true),
        // "Enables STrPe-DS")(
        //("strpe_log", "Logging STrPe & STrPe-DS specific data to file.")
#if defined __DBS__
        ("monitor", "The peer is a monitor")
#endif
	;

    }

    boost::program_options::variables_map vm;

    try {
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    } catch (std::exception& e) {
      // If the argument passed is unknown, print the list of available arguments
      std::cout << desc << "\n";
      return 1;
    }

    boost::program_options::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      return 1;
    }

    // }}}

    class Console console;
    
    if (vm.count("player_port")) {
      console.SetPlayerPort(vm["player_port"].as<uint16_t>());
      TRACE("Player port = "
	    << console.GetPlayerPort());
    }

    console.WaitForThePlayer();
    TRACE("Player connected");

    if (vm.count("splitter_addr")) {
      console.SetSplitterAddr(ip::address::from_string(vm["splitter_addr"].as<std::string>()));
      TRACE("Splitter address = "
	    << console.GetSplitterAddr());
    }

    if (vm.count("splitter_port")) {
      // {{{
      
      console.SetSplitterPort(vm["splitter_port"].as<uint16_t>());
      TRACE("Splitter port = "
	    << console.GetSplitterPort());
      
      // }}}
    }
    
    console.ConnectToTheSplitter();
    TRACE("Connected to the splitter");

    console.ReceiveSourceEndpoint();
    TRACE("Source = ("
	  << console.GetSourceAddr()
	  << ","
	  << std::to_string(console.GetSourcePort())
	  << ")");

    console.ConnectToTheSource();
    TRACE("Connected to the source");

    console.ReceiveChannel();
    /*if (vm.count("channel")) {
      console.SetChannel(vm["channel"].as<std::string>());
      TRACE("Channel = "
	    << console.GetChannel());
	    }*/

    console.ReceiveHeaderLength();
    TRACE("Header length = "
	    << console.GetHeaderLength());
        /*if (vm.count("header_length")) {
      console.SetHeaderSize(vm["header_length"].as<int>());
      TRACE("Header size = "
	    << console.GetHeaderSize());
	    }*/

    console.RelayHeader();
    TRACE("Header relayed");
    
    console.ReceiveChunkSize();
    TRACE("Chunk size = "
	  << console.GetChunkSize());

    console.ReceiveBufferSize();
    TRACE("Buffer size = "
	  << console.GetBufferSize());

#if defined __IMS__

    TRACE("Using IMS");
    
    console.ReceiveMcastGroup();
    TRACE("Using IP multicast group = ("
	  << console.GetMcastAddr().to_string()
	  << ","
	  << console.GetMcastPort()
	  << ")");

#elif defined _DBS_

    TRACE("Using DBS");

    if (vm.count("max_chunk_debt")) {
      // {{{
      
      console.SetMaxChunkDebt(vm["max_chunk_debt"].as<int>());
      TRACE("Maximum chunk debt = "
	    << Console.GetMaxChunkDebt());
      
      // }}}
    }

    if (vm.count("team_port")) {
      // {{{
      
      console.SetTeamPort(vm["team_port"].as<uint16_t>());
      TRACE("team_port = "
	    << console.GetTeamPort());
      
      // }}}
    }

    if (vm.count("use_localhost")) {
      // {{{
      
      console.SetUseLocalHost(true);
      TRACE("use_localhost = "
	    << console.GetUseLocalHost());
      
      // }}}
    }

#eise if defined _NTS_

    TRACE("Using NTS");

    if (vm.count("source_port_step")) {
      Console.SetPortStep(vm["source_port_step"].as<int>());
    }
    TRACE("Source port step ="
	  << Console.GetPortStep());
    
#endif
    
    console.Init();

#if defined __IMS__
    
    console.ListenToTheTeam();

#else
    
    console.ListenToTheTeam();
    TRACE("Listening to the team");
    
    //console.ReceiveTheNumberOfPeers();
    console.ReceiveTheListOfPeers();
    TRACE("List of peers received");
    TRACE("Number of peers in the team (excluding me) ="
	  << std::to_string(console.GetNumberOfPeers()));

#endif    

    console.DisconnectFromTheSplitter();
    TRACE("Recived the configuration from the splitter.");
    TRACE("Clossing the connection");

    TRACE("Buffering ...");
    console.BufferData();
    TRACE("Buffering done");

    console.Start();
    TRACE("Peer running in a thread");
    
    LOG("+-----------------------------------------------------+");
    LOG("| Received = Received kbps, including retransmissions |");
    LOG("|     Sent = Sent kbps                                |");
    LOG("|       (Expected values are between parenthesis)     |");
    LOG("------------------------------------------------------+");
    LOG("");
    LOG("         |     Received (kbps) |          Sent (kbps) |");
    LOG("    Time |      Real  Expected |       Real  Expected | Team description");
    LOG("---------+---------------------+----------------------+-----------------------------------...");

    int last_chunk_number = console.GetPlayedChunk();
    float kbps_expected_recv = 0.0f;
    float kbps_recvfrom = 0.0f;
#if not defined __IMS__
    int last_recvfrom_counter = console.GetRecvfromCounter();
    int last_sendto_counter = -1;
    if (console.GetSendtoCounter() < 0) {
      last_sendto_counter = 0;
    } else {
      //console.SetSendtoCounter(0);
      last_sendto_counter = 0;
    }
    float team_ratio = 0.0f;
    float kbps_expected_sent = 0.0f;
    float kbps_sendto = 0.0f;
    // float nice = 0.0f;
    int counter = 0;
#endif

    while (console.IsPlayerAlive()) {
      boost::this_thread::sleep(boost::posix_time::seconds(1));
      kbps_expected_recv = ((console.GetPlayedChunk() - last_chunk_number) *
                            console.GetChunkSize() * 8) / 1000.0f;
      last_chunk_number = console.GetPlayedChunk();
#if not defined __IMS__
      kbps_recvfrom = ((console.GetRecvfromCounter() - last_recvfrom_counter) *
                       console.GetChunkSize() * 8) / 1000.0f;
      last_recvfrom_counter = console.GetRecvfromCounter();
      team_ratio = console.GetPeerList()->size() / (console.GetPeerList()->size() + 1.0f);
      kbps_expected_sent = (int)(kbps_expected_recv * team_ratio);
      kbps_sendto = ((console.GetSendtoCounter() - last_sendto_counter) *
                     console.GetChunkSize() * 8) / 1000.0f;
      last_sendto_counter = console.GetSendtoCounter();

      if (kbps_recvfrom > 0 and kbps_expected_recv > 0) {
        // nice = 100.0 / (kbps_expected_recv / kbps_recvfrom) *
        // (console.GetPeerList()->size() + 1.0f);
      } else {
        // nice = 0.0f;
      }
      LOG("|");

      if (kbps_expected_recv < kbps_recvfrom) {
        LOG(_SET_COLOR(_RED));
      } else if (kbps_expected_recv > kbps_recvfrom) {
        LOG(_SET_COLOR(_GREEN));
      }
#endif

      // TODO: Format default options
      boost::format format("Defaut = %5i");
      
      // TODO: format
      LOG(kbps_expected_recv);
      LOG(kbps_recvfrom);
      //#print(("{:.1f}".format(nice)).rjust(6), end=' | ')
      //#sys.stdout.write(Color.none)
#ifndef __IMS__
      if (kbps_expected_sent > kbps_sendto) {
        LOG(_SET_COLOR(_RED));
      } else if (kbps_expected_sent < kbps_sendto) {
        LOG(_SET_COLOR(_GREEN));
      }
      // TODO: format
      LOG(kbps_sendto);
      LOG(kbps_expected_sent);
      // sys.stdout.write(Color.none)
      // print(repr(nice).ljust(1)[:6], end=' ')
      LOG(console.GetPeerList()->size());
      counter = 0;
      for (std::vector<boost::asio::ip::udp::endpoint>::iterator p = console.GetPeerList()->begin(); p != console.GetPeerList()->end(); ++p) {
        if (counter < 5) {
          LOG("("
	      << p->address().to_string()
	      << ","
	      << std::to_string(p->port())
              << ")");
          counter++;
        } else {
          break;
          LOG("");
        }
      }
#endif

    }

    return 0;
  }
}


int main(int argc, const char* argv[]) {
  //try {
    return p2psp::run(argc, argv);
    //} catch (boost::system::system_error e) {
    //TRACE(e.what());
    //}

  return -1;
}
