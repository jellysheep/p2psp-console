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

//#include <boost/format.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <iostream>
#include <chrono>
//using namespace std;
//using ns = chrono::nanoseconds;
//using get_time = chrono::steady_clock;

#include "common.h"
#include "core/common.h"

#if defined __IMS__
#include "core/peer_ims.h"
#endif   /* __IMS__ */

#if defined __DBS__ || defined __ACS__
#if defined __monitor__
#include "core/monitor_dbs.h"    // Includes peer_dbs.h
#else
#include "core/peer_dbs.h"
#endif   /* __monitor__ */
#endif   /* __DBS__ || __ACS__ */

#if defined __LRS__
#if defined __monitor__
#include "core/monitor_lrs.h"   // Includes monitor_dbs.h
#else
#include "core/peer_dbs.h"
#endif   /* __monitor__ */
#endif   /* __LRS__ */

#if defined __NTS__
#if defined __monitor__
#include "core/monitor_nts.h"   // Includes Peer_NTS
#else
//#include "core/peer_nts.h"      // Includes peer_dbs.h
#include "core/peer_symsp.h"    // Includes peer_nts.h, which includes peer_dbs.h
#endif    /* __monitor__ */
#endif    /* __NTS__ */

#include "util/trace.h"

// }}}

namespace p2psp {
  using namespace std;
  using namespace boost;
  
  class Console:

#if defined __IMS__  
    public Peer_IMS
#endif

#if defined __DBS__ || defined __ACS__
#if defined __monitor__
    public Monitor_DBS
#else
    public Peer_DBS
#endif /* __monitor__ */
#endif /* __DBS__ || __ACS__ */

#if defined __LRS__
#if defined __monitor__
    public Monitor_LRS
#else
    public Peer_DBS
#endif
#endif /* __LRS__ */
  
#if defined __NTS__
#if defined __monitor__
    public Monitor_NTS
#else
    public Peer_SYMSP
#endif /* __monitor__ */
#endif /* __NTS__ */

  {
    //  class Console: public Peer_core {
    // {{{
  protected:
    // {{{
    
    struct Source {
      ip::address addr;
      PORT_TYPE port;
    };
    
    PORT_TYPE player_port_;
    io_service io_service_;
    ip::tcp::acceptor acceptor_;
    ip::tcp::socket source_socket_;
    ip::tcp::socket player_socket_;
    HEADER_SIZE_TYPE header_size_;
    struct Source source;
    std::string GET_message_;
    std::string channel_;

    // }}}
  public:
    // {{{
    
    Console() : io_service_(),
		acceptor_(io_service_),
		source_socket_(io_service_),
		player_socket_(io_service_) {
      // {{{

      //header_size_ = GetDefaultHeaderSize();
      //channel_ = GetDefaultChannel();
      //SetGETMessage(channel_);
#if defined __D__
      TRACE("Console initialized");
#endif
      // }}}
    }

    ~Console() {}

    void ReceiveSourceEndpoint() {
      // {{{

      boost::array<char, 6> buffer;
      read(splitter_socket_, ::buffer(buffer));
      
      char *raw_data = buffer.data();
      
      in_addr ip_raw = *(in_addr *)(raw_data);
      source.addr = ip::address::from_string(inet_ntoa(ip_raw));
      source.port = ntohs(*(short *)(raw_data + 4));
#if defined __D__
      TRACE("source_endpoint = ("
	    << source.addr.to_string()
	    << ","
	    << std::to_string(source.port)
	    << ")");
#endif
      // }}}
    }

    ip::address GetSourceAddr() {
      // {{{
      
      return source.addr;

      // }}}
    }
    
    PORT_TYPE GetSourcePort() {
      // {{{
      
      return source.port;

      // }}}
    }

    void SetGETMessage() {
      // {{{
      
      std::stringstream ss;
      ss << "GET /" << channel_ << " HTTP/1.1\r\n"
	 << "\r\n";
      GET_message_ = ss.str();
#if defined __D__
      TRACE("GET_message = "
	    << GET_message_);
#endif
      ss.str("");

      // }}}
    }

    void ReceiveChannel() {
      // {{{

      unsigned short channel_size; {
	std::vector<char> message(2);
	read(splitter_socket_, boost::asio::buffer(message/*,2*/));
	channel_size = ntohs(*(short *)(message.data()));
      }
#if defined __D__
      TRACE("channel_size = "
	    << channel_size);
#endif
      {
	std::vector<char> messagex(channel_size);
	boost::asio::read(splitter_socket_, boost::asio::buffer(messagex/*, channel_size*/));
      
	channel_ = std::string(messagex.data(), channel_size);
      }
#if defined __D__
      TRACE("channel = "
	    << channel_);
#endif
      SetGETMessage();

      // }}}
    }

    std::string GetChannel() {
      // {{{

      return channel_;
      
      // }}}
    }

    void ReceiveHeaderSize() {
      // {{{

      boost::array<char, 2> buffer;
      read(splitter_socket_, ::buffer(buffer));
      
      header_size_ = ntohs(*(short *)(buffer.c_array()));
      
#if defined __D__
      TRACE("header_size (in bytes) = "
	    << std::to_string(header_size_));
#endif
      // }}}
    }

    HEADER_SIZE_TYPE GetHeaderSize() {
      // {{{
      
      return header_size_;

      // }}}
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
#if defined __D__      
      TRACE("Connected to the source at ("
	    << this->source.addr.to_string()
	    << ","
	    << std::to_string(this->source.port)
	    << ") from "
	    << source_socket_.local_endpoint().address().to_string());
#endif
      // }}}
    }

    void RequestHeader() {
      source_socket_.send(asio::buffer(GET_message_));      
    }
    
    void RelayHeader() {
      // {{{
            
      boost::array<char, 128> buf;
      //boost::system::error_code error;
      for(int header_load_counter_ = 0; header_load_counter_ < GetHeaderSize();) {

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
      
      ip::tcp::endpoint endpoint(ip::tcp::v4(), player_port_);

      acceptor_.open(endpoint.protocol());
      acceptor_.set_option(ip::tcp::acceptor::reuse_address(true));
      acceptor_.bind(endpoint);
      acceptor_.listen();
      std::cout
	<< "Waiting for the player at ("
	<< endpoint.address().to_string()
	<< ","
	<< std::to_string(endpoint.port())
	<< ")"
	<< std::endl;
      acceptor_.accept(player_socket_);
#if defined __D__
      TRACE("Player connected. Player is ("
            << player_socket_.remote_endpoint().address().to_string()
	    << ","
            << std::to_string(player_socket_.remote_endpoint().port())
	    << ")");
#endif      
      // }}}
    }

    bool PlayChunk(int chunk) {
      // {{{

      try {
        write(player_socket_, buffer(chunk_ptr[chunk % buffer_size_].data));
        return true;
      } catch (std::exception e) {
	std::cout
	  << "Player disconnected"
	  << std::endl;
	//std::cout << e.what() << std::endl;
        //player_alive_ = false;
	return false;
	//return true;
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
    // {{{

    // {{{ Argument Parsing

    const char description[80] = "This is a peer node of a P2PSP team.\n"
      "Parameters";

    boost::program_options::options_description desc(description);

    {

      uint16_t player_port = Console::GetDefaultPlayerPort();
      std::string splitter_addr = p2psp::Peer_core::GetDefaultSplitterAddr().to_string();
      uint16_t splitter_port = p2psp::Peer_core::GetDefaultSplitterPort();
#if not defined __IMS__
      int max_chunk_debt = p2psp::Peer_DBS::GetDefaultMaxChunkDebt();
      uint16_t team_port = p2psp::Peer_core::GetDefaultTeamPort();
#endif
#if defined __NTS__ && not defined __monitor__
      int source_port_step = 0;
#endif

      // TODO: strpe option should expect a list of arguments, not bool
      desc.add_options()
        ("help,h", "Produce this help message and exits.")
#if not defined __IMS__
        ("max_chunk_debt", boost::program_options::value<int>()->default_value(max_chunk_debt), "Maximum number of times that other peer can not send a chunk to this peer.")
#endif
        ("player_port", boost::program_options::value<uint16_t>()->default_value(player_port), "Port to communicate with the player.")
#if defined __NTS__ && not defined __monitor__
        ("source_port_step", boost::program_options::value<int>()->default_value(source_port_step), "Source port step forced when behind a sequentially port allocating NAT (conflicts with --chunk_loss_period).")
#endif
        ("splitter_addr", boost::program_options::value<std::string>()->default_value(splitter_addr), "IP address or hostname of the splitter.")
        ("splitter_port", boost::program_options::value<uint16_t>()->default_value(splitter_port), "Listening port of the splitter.")
#if not defined __IMS__
        ("team_port", boost::program_options::value<uint16_t>()->default_value(team_port), "Port to communicate with the peers. By default the OS will chose it.")
        ("use_localhost", "Forces the peer to use localhost instead of the IP of the adapter to connect to the splitter." "Notice that in this case, peers that run outside of the host will not be able to communicate with this peer.")
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

#if defined __IMS__
    std::cout << "Using Peer_IMS" << std::endl;
#endif
    
#if defined __DBS__ || defined __ACS__
#if defined __monitor__
    std::cout << "Using Monitor_DBS" << std::endl;
#else
    std::cout << "Using Peer_DBS" << std::endl;
#endif /* __monitor__ */
#endif /* __DBS__ || __ACS__ */

#if defined __LRS__
#if defined __monitor__
    std::cout << "Using Monitor_LRS" << std::endl;
#else
    std::cout << "Using Peer_DBS" << std::endl;
#endif
#endif /* __LRS__ */
    
#if defined __NTS__
#if defined __monitor__
    std::cout << "Using Monitor_NTS" << std::endl;
#else
    std::cout << "Using Peer_NTS" << std::endl;
#endif /* __monitor__ */
#endif /* __NTS__ */

    // {{{ Peer instantiation
    
    class Console* peer = new Console();

    // }}}
    
    if (vm.count("player_port")) {
      // {{{

      peer->SetPlayerPort(vm["player_port"].as<uint16_t>());
#if defined __D__
      TRACE("Player port = "
	  << peer->GetPlayerPort());
#endif
      
      // }}}
    }

    peer->WaitForThePlayer();
    std::cout
      << "Player connected"
      << std::endl;

    if (vm.count("splitter_addr")) {
      // {{{

      peer->SetSplitterAddr(ip::address::from_string(vm["splitter_addr"].as<std::string>()));
#if defined __D__
      TRACE("Splitter address = "
	    << peer->GetSplitterAddr());
#endif
      
      // }}}
    }

    if (vm.count("splitter_port")) {
      // {{{
      
      peer->SetSplitterPort(vm["splitter_port"].as<uint16_t>());
#if defined __D__
      TRACE("Splitter port = "
	  << peer->GetSplitterPort());
#endif
      
      // }}}
    }
    
    peer->ConnectToTheSplitter();
#if defined __D__
    TRACE("Connected to the splitter");
#endif
    /*std::cout
      << "Real splitter port = "
      << peer->GetRealSplitterPort()
      << std::endl;*/

    peer->ReceiveSourceEndpoint();
#if defined __D__
    TRACE("Source = ("
	  << peer->GetSourceAddr()
	  << ","
	  << std::to_string(peer->GetSourcePort())
	  << ")");
#endif
    
    peer->ConnectToTheSource();
#if defined __D__
    TRACE("Connected to the source");
#endif
    
    peer->ReceiveChannel();
#if defined __D__
    TRACE("channel = "
	  << peer->GetChannel());
#endif
    
    peer->ReceiveHeaderSize();
#if defined __D__
    TRACE("Header size = "
	  << peer->GetHeaderSize());
#endif
    
    peer->RequestHeader();
#if defined __D__
    TRACE("Header requested");
#endif

    std::cout << "Relaying the header from the source to the player ... " << std::flush;
    peer->RelayHeader();
    std::cout << "done" << std::endl;
    
    peer->ReceiveChunkSize();
#if defined __D__
    TRACE("Chunk size = "
	  << peer->GetChunkSize());
#endif
    
    peer->ReceiveBufferSize();
#if defined __D__
    TRACE("Buffer size = "
	  << peer->GetBufferSize());
#endif
    
#if defined __IMS__
    // {{{
    
    peer->ReceiveMcastGroup();
#if defined __D__
    TRACE("Using IP multicast group = ("
	  << peer->GetMcastAddr().to_string()
	  << ","
	  << peer->GetMcastPort()
	  << ")");
#endif
    
    // }}}

#else /* __IMS__ */
    
    // {{{

    if (vm.count("max_chunk_debt")) {
      // {{{
      
      peer->SetMaxChunkDebt(vm["max_chunk_debt"].as<int>());
#if defined __D__
      TRACE("Maximum chunk debt = "
	    << peer->GetMaxChunkDebt());
#endif
      
      // }}}
    }

    if (vm.count("team_port")) {
      // {{{
      
      peer->SetTeamPort(vm["team_port"].as<uint16_t>());
#if defined __D__
      TRACE("team_port = "
	    << peer->GetTeamPort());
#endif
      
      // }}}
    }

    if (vm.count("use_localhost")) {
      // {{{
      
      peer->SetUseLocalHost(true);
#if defined __D__
      TRACE("use_localhost = "
	    << peer->GetUseLocalHost());
#endif
      
      // }}}
    }

    // }}}
#endif /* __IMS__*/
    
#if defined __NTS__
# if not defined __monitor__
    // {{{

    if (vm.count("source_port_step")) {
      peer->SetPortStep(vm["source_port_step"].as<int>());
    }
#if defined __D__
    TRACE("Source port step = "
	  << peer->GetPortStep());
#endif
    
    // }}}
#endif
#endif

    peer->Init();    
    peer->ListenToTheTeam();
#if defined __D__
    TRACE("Listening to the team");
#endif
    
#if not defined __IMS__
    // {{{
    
#if defined __D__
    TRACE("Receiving the list of peers ... ");
#endif
    peer->ReceiveTheListOfPeers();
#if defined __D__
    std::cout << "done" << std::endl;
    TRACE("List of peers received");
    TRACE("Number of peers in the team (excluding me) = "
	<< std::to_string(peer->GetNumberOfPeers()));    
#endif
    
    // }}}
#endif    

    peer->SendReadyForReceivingChunks();
    
    peer->DisconnectFromTheSplitter();
#if defined __D__
    TRACE("Recived the configuration from the splitter.");
    TRACE("Clossing the connection");
#endif
    
    std::cout
      << "Buffering ... "
      << std::endl << std::flush; {
      //time_t start_time = time(NULL);
      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
      //auto start = std::chrono::steady_clock::now();
      peer->BufferData();
      std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
      /*auto duration = std::chrono::duration_cast<std::chrono::milliseconds> 
	(std::chrono::steady_clock::now() - start);*/
      std::cout << "done" << std::endl;
      std::cout
	<< "Buffering time = "
	<< std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()/1000000.0
	<< " seconds" << std::endl;
    }
    peer->Start();
    //LOG("Peer running in a thread");

    std::cout << _RESET_COLOR();

#if defined __IMS__
    
    std::cout << "                     | Received |     Sent |" << std::endl;
    std::cout << "                Time |   (kbps) |   (kbps) |" << std::endl;
    std::cout << "---------------------+----------+----------+" << std::endl;

#else

    std::cout << std::endl;
    std::cout << "                     | Received Expected |     Sent Expected | Team |" << std::endl;
    std::cout << "                Time |   (kbps)   (kbps) |   (kbps)   (kbps) | size | Peer list" << std::endl;
    std::cout << "---------------------+-------------------+-------------------+------+----------..." << std::endl;

#endif
    
    int kbps_recvfrom = 0;
    int kbps_sendto = 0;
    int last_sendto_counter = -1;
    int last_recvfrom_counter = peer->GetRecvfromCounter();
    
#if not defined __IMS__

    int last_chunk_number = peer->GetPlayedChunk();
    int kbps_expected_recv = 0;
    if (peer->GetSendtoCounter() < 0) {
      last_sendto_counter = 0;
    } else {
      //peer->SetSendtoCounter(0);
      last_sendto_counter = 0;
    }
    float team_ratio = 0.0f;
    int kbps_expected_sent = 0;
    int counter = 0;

#endif

    while (peer->IsPlayerAlive()) {
      boost::this_thread::sleep(boost::posix_time::seconds(1));

      { /* Print current time */
	using boost::posix_time::ptime;
	using boost::posix_time::second_clock;
	using boost::posix_time::to_simple_string;
	using boost::gregorian::day_clock;
	ptime todayUtc(day_clock::universal_day(), second_clock::universal_time().time_of_day());
	std::cout << to_simple_string(todayUtc);
      }

      std::cout
	<< " |";

      kbps_sendto = int(((peer->GetSendtoCounter() - last_sendto_counter) *
			 peer->GetChunkSize() * 8) / 1000.0f);
      last_sendto_counter = peer->GetSendtoCounter();
      kbps_recvfrom = int(((peer->GetRecvfromCounter() - last_recvfrom_counter) *
			   peer->GetChunkSize() * 8) / 1000.0f);
      last_recvfrom_counter = peer->GetRecvfromCounter();

#if not defined __IMS__

      kbps_expected_recv = int(((peer->GetPlayedChunk() - last_chunk_number) *
				peer->GetChunkSize() * 8) / 1000.0f);
      last_chunk_number = peer->GetPlayedChunk();
      {
	team_ratio = peer->GetPeerList()->size() / (peer->GetPeerList()->size() + 1.0f);
      }
      kbps_expected_sent = (int)(kbps_expected_recv * team_ratio);

      if (kbps_expected_recv < kbps_recvfrom) {
	std::cout <<_SET_COLOR(_GREEN);
      } else if (kbps_expected_recv > kbps_recvfrom) {
	std::cout << _SET_COLOR(_RED);
      }

#endif /* not defined __IMS__ */
    
      std::cout
	<< std::setw(9)
	<< kbps_recvfrom
	<< _RESET_COLOR();

#if not defined __IMS__
    
      std::cout
	<< std::setw(9)
	<< kbps_expected_recv;

      std::cout
	<< " |";

      if (kbps_expected_sent < kbps_sendto) {
	std::cout <<_SET_COLOR(_GREEN);
      } else if (kbps_expected_sent > kbps_sendto) {
	std::cout << _SET_COLOR(_RED);
      }

#endif /* not defined __IMS__ */
    
      std::cout
	<< std::setw(9)
	<< kbps_sendto
	<< _RESET_COLOR();
	
#if not defined __IMS__
    
      std::cout
	<< std::setw(9)
	<< kbps_expected_sent;
      
#endif /* not defined __IMS__ */
    
      std::cout << " |";
    
#ifndef __IMS__

      {
	std::cout
	  << std::setw(5)
	  << peer->GetPeerList()->size()
	  << " | ";
	counter = 0;
	for (std::vector<boost::asio::ip::udp::endpoint>::iterator p = peer->GetPeerList()->begin();
	     p != peer->GetPeerList()->end();
	     ++p) {
	  if (counter < 5) {
	    std::cout << "("
		      << p->address().to_string()
		      << ","
		      << std::to_string(p->port())
		      << ")";
	    counter++;
	  } else {
	    break;
	    std::cout << "";
	  }
	}
      }

#endif
      std::cout
	<< std::endl;
    }
    
    return 0;
  }

    // }}}
}

int main(int argc, const char* argv[]) {
  
  try {
    return p2psp::run(argc, argv);
  } catch (boost::system::system_error e) {
    TRACE(e.what());
  }
  return -1;
  
}
