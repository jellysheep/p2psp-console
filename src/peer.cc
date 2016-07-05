//
//  peer.cc -- Console version of a P2PSP peer
//
//  This code is distributed under the GNU General Public License (see
//  THE_GENERAL_GNU_PUBLIC_LICENSE.txt for extending this information).
//
//  Copyright (C) 2016, the P2PSP team.
//  http://www.p2psp.org
//
//  This program waits for the connection of the player, retrieves the
//  header from the source (and posiblely some stream), retrieves the
//  team configuration from the splitter and finally, runs a peer.
//

#define __IMS__

// {{{ includes

#include <boost/format.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "core/common.h"
//#include "core/monitor_dbs.h"
//#include "core/monitor_lrs.h"
//#include "core/monitor_nts.h"
//#include "core/peer_core.h"
#include "core/peer_dbs.cc"
#include "core/peer_ims.cc"
//#include "core/peer_nts.h"
//#include "core/peer_symsp.h"
//#include "peer_strpeds.h"
//#include "peer_strpeds_malicious.h"
//#include "trusted_peer.h"
//#include "malicious_peer.h"
#include "util/trace.h"

// }}}

namespace p2psp {

#if defined __IMS__  
  class Console: public Peer_IMS {
#else
#endif
    // {{{

  protected:

    struct Source {
      ip::address addr;
      uint16_t port;
    };
    
    static const uint16_t kPlayerPort = 9999;
    
    uint16_t player_port_ = kPlayerPort;
    io_service io_service_;
    ip::tcp::acceptor acceptor_;
    ip::tcp::socket source_socket_;
    ip::tcp::socket player_socket_;
    int header_size;
    struct Source source;
    
  public:

    Console() : io_service_(),
	       acceptor_(io_service_),
	       source_socket_(io_service_),
	       player_socket_(io_service_) {
    }

    void SetSourceAddr(ip::address addr) {
      // {{{

      source.addr = addr;

      // }}}
    }

    ip::address GetSourceAddr() {
      return source.addr;
    }
    
    static ip::address GetDefaultSourceAddr() {
      // {{{

      return ip::address::from_string(/*kSourceAddrStr*/"127.0.0.1");

      // }}}
    }

    void SetSourcePort(uint16_t port) {
      // {{{

      source.port = port;

      // }}}
    }

    uint16_t GetSourcePort() {
      return source.port;
    }
    
    static uint16_t GetDefaultSourcePort() {
      // {{{

      return 8000;

      // }}}
    }

    void ConnectToTheSource() throw(boost::system::system_error) {
      // {{{

      ip::tcp::endpoint source_ep(this->source.addr, this->source.port);
      source_socket_.connect(source_ep);
      TRACE("Connected to the source at ("
	    << this->source.addr.to_string()
	    << ","
	    << std::to_string(this->source.port)
	    << ") from "
	    << source_socket_.local_endpoint().address().to_string());

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
#ifdef _1_
    void ReceiveHeaderSize() {
      // {{{

      boost::array<char, 2> buffer;
      read(splitter_socket_, ::buffer(buffer));
      header_size_in_bytes_ = ntohs(*(short *)(buffer.c_array()));
      TRACE("header_size (in bytess) = "
	    << std::to_string(header_size_in_bytess_));

      // }}}
    }
#endif

    void SetHeaderSize(int header_size) {
      // {{{

      this->header_size = header_size;

      // }}}
    }

    int GetHeaderSize() {
      // {{{
      
      return header_size;
      
      // }}}
    }

    void RelayHeader() {
      // {{{
      
      std::vector<char> header(header_size);
      boost::system::error_code ec;
      streambuf chunk;
      read(splitter_socket_, chunk, transfer_exactly(header_size), ec);
      
      if (ec) {
	ERROR(ec.message());
	splitter_socket_.close();
	//ConnectToTheSplitter();
      }
      
      try {
	write(player_socket_, chunk);
      } catch (std::exception e) {
	ERROR(e.what());
	ERROR("error sending data to the player");
	TRACE("len(data) =" << std::to_string(chunk.size()));
	boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
      }
      
      TRACE("Received "
	    << std::to_string(header_size)
	    << " bytes of header");
      
      // }}}
    }
    
    bool PlayChunk(std::vector<char> chunk) {
      // {{{

      try {
        write(player_socket_, /*buffer(chunks_[chunk_number % buffer_size_].data*/buffer(chunk));
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

      return kPlayerPort;

      // }}}
    }

    // }}}
  };

  int run(int argc, const char* argv[]) throw(boost::system::system_error) {

    // {{{ Argument Parsing

    boost::program_options::options_description desc("This is the peer node of a P2PSP team.\n" "Parameters");

    {

      uint16_t player_port = Console::GetDefaultPlayerPort();
      std::string source_addr = Console::GetDefaultSourceAddr().to_string();
      uint16_t source_port = Console::GetDefaultSourcePort();
      std::string splitter_addr = p2psp::Peer_core::GetDefaultSplitterAddr().to_string();
      uint16_t splitter_port = p2psp::Peer_core::GetDefaultSplitterPort();
      int max_chunk_debt = p2psp::Peer_DBS::GetDefaultMaxChunkDebt();
      uint16_t team_port = p2psp::Peer_core::GetDefaultTeamPort();
      int source_port_step = 0;

      // TODO: strpe option should expect a list of arguments, not bool
      desc.add_options()
        ("help,h", "Produce this help message and exits.")
        ("enable_chunk_loss",
         boost::program_options::value<std::string>(),
         "Forces a lost of chunks.")
        ("max_chunk_debt",
         boost::program_options::value<int>()->default_value(max_chunk_debt),
         "Maximum number of times that other peer can not send a chunk to this peer.")
        ("player_port",
         boost::program_options::value<uint16_t>()->default_value(player_port),
         "Port to communicate with the player.")
        ("source_port_step",
         boost::program_options::value<int>()->default_value(source_port_step),
         "Source port step forced when behind a sequentially port allocating NAT (conflicts with --chunk_loss_period).")
        ("source_addr",
         boost::program_options::value<std::string>()->default_value(source_addr),
         "IP address or hostname of the source.")
        ("source_port",
         boost::program_options::value<uint16_t>()->default_value(source_port),
         "Listening port of the source.")
        ("splitter_addr",
         boost::program_options::value<std::string>()->default_value(splitter_addr),
         "IP address or hostname of the splitter.")
        ("splitter_port",
         boost::program_options::value<uint16_t>()->default_value(splitter_port),
         "Listening port of the splitter.")
        ("team_port",
         boost::program_options::value<uint16_t>()->default_value(team_port),
         "Port to communicate with the peers. By default the OS will chose it.")
        ("use_localhost",
         "Forces the peer to use localhost instead of the IP of the adapter to connect to the splitter."
         "Notice that in this case, peers that run outside of the host will not be able to communicate with this peer.")
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
        ("monitor",
         "The peer is a monitor")
        ("show_buffer",
         "Shows the status of the buffer of chunks.");

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

    if (vm.count("header_size")) {
      console.SetHeaderSize(vm["header_size"].as<int>());
      TRACE("Header size = "
	    << console.GetHeaderSize());
    }

    console.WaitForThePlayer();
    TRACE("Player connected");

    if (vm.count("source_addr")) {
      console.SetSourceAddr(ip::address::from_string(vm["source_addr"].as<std::string>()));
      TRACE("Source address = "
	    << console.GetSourceAddr());
    }

    if (vm.count("source_port")) {
      // {{{
      
      console.SetSourcePort(vm["source_port"].as<uint16_t>());
      TRACE("Source port = "
	    << console.GetSourcePort());
      
      // }}}
    }
    
    console.ConnectToTheSource();
    TRACE("Connected to the source");

    console.RelayHeader();
    TRACE("Header relayed");

    if (vm.count("splitter_addr")) {
      console.SetSplitterAddr(ip::address::from_string(vm["splitter_addr"].as<std::string>()));
      TRACE("Splitter address = "
	    << console.GetSourceAddr());
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

#if defined __IMS__
    TRACE("Using IMS");
    
    console.ReceiveMcastChannel();
    TRACE("Using IP multicast channel = ("
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

    console.ReceiveChunkSize();
    TRACE("Chunk size = "
	  << console.GetChunkSize());

    console.ReceiveBufferSize();
    TRACE("Buffer size = "
	  << console.GetBufferSize());
    
    console.Init();

#if defined __IMS__
    console.ListenToTheTeam();
#else
    // {{{
    
    console.ReceiveTheNumberOfPeers();
    TRACE("Number of peers in the team (excluding me) ="
	  << std::to_string(console.GetNumberOfPeers()));
    
    console.ListenToTheTeam();
    TRACE("Listening to the team");
    
    console.ReceiveTheListOfPeers();
    TRACE("List of peers received");
#endif    

    console.DisconnectFromTheSplitter();
    TRACE("Recived the configuration from the splitter.");
    TRACE("Clossing the connection");
    
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
#ifndef __IMS__
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
#ifndef __IMS__
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
  try {
    return p2psp::run(argc, argv);
  } catch (boost::system::system_error e) {
    TRACE(e.what());
  }

  return -1;
}
