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
#include "core/monitor_lrs.h"   // Includes monitor_dbs.h
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
  
  class Console: /*public Peer_core {*/

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
    public Monitor_LRS
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
      TRACE("Console initialized");

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
      
      TRACE("source_endpoint = ("
	    << source.addr.to_string()
	    << ","
	    << std::to_string(source.port)
	    << ")");

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

      TRACE("GET_message = "
	    << GET_message_);

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
      
      TRACE("channel_size = "
	    << channel_size);

      {
	std::vector<char> messagex(channel_size);
	boost::asio::read(splitter_socket_, boost::asio::buffer(messagex/*, channel_size*/));
      
	channel_ = std::string(messagex.data(), channel_size);
      }
      //channel_ = "BBB-143.ogv";
      TRACE("length = "
	    << channel_.length());
      TRACE("channel = "
	    << channel_);
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
      
      TRACE("header_size (in bytes) = "
	    << std::to_string(header_size_));

      // }}}
    }
  
    /*static std::string GetDefaultChannel() {
      return "BBB-143.ogv";
      }*/

    /*static int GetDefaultHeaderSize() {
      return 4096;
      }*/

    /*void SetChannel(std::string channel) {
      // {{{

      channel_ = channel;
      SetGETMessage(channel_);

      // }}}
      }*/

    /*void SetSourceAddr(ip::address addr) {
      // {{{

      source.addr = addr;

      // }}}
      }*/

    /*static ip::address GetDefaultSourceAddr() {
      // {{{

      return ip::address::from_string("127.0.0.1");

      // }}}
      }*/

    /*void SetSourcePort(uint16_t port) {
      // {{{

      source.port = port;

      // }}}
      }*/

    HEADER_SIZE_TYPE GetHeaderSize() {
      // {{{
      
      return header_size_;

      // }}}
    }
    
    /*static uint16_t GetDefaultSourcePort() {
      // {{{

      return 8000;

      // }}}
      }*/

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
    
    /*void SetHeaderSize(int header_size) {
      // {{{

      this->header_size_ = header_size;

      // }}}
      }*/

    bool PlayChunk(/*std::vector<char> chunk*/int chunk_number) {
      // {{{

      try {
        write(player_socket_, buffer(chunks_[chunk_number % buffer_size_].data));
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
    // {{{

    // {{{ Argument Parsing

    const char description[80] = "This is the peer node of a P2PSP team.\n"
#if defined __IMS__
      "Using IMS.\n"
#elif defined __DBS__
      "Using DBS.\n"
#elif defined __ACS__
      "Using ACS.\n"
#elif defined __LRS__
      "Using LRS.\n"
#elif defined __NTS__
      "Using NTS.\n"
#endif
      "Parameters";

    boost::program_options::options_description desc(description);

    {

      uint16_t player_port = Console::GetDefaultPlayerPort();
      //      std::string source_addr = Console::GetDefaultSourceAddr().to_string();
      //      uint16_t source_port = Console::GetDefaultSourcePort();
      std::string splitter_addr = p2psp::Peer_core::GetDefaultSplitterAddr().to_string();
      uint16_t splitter_port = p2psp::Peer_core::GetDefaultSplitterPort();
#if not defined __IMS__
      int max_chunk_debt = p2psp::Peer_DBS::GetDefaultMaxChunkDebt();
      uint16_t team_port = p2psp::Peer_core::GetDefaultTeamPort();
#endif
#if defined __NTS__
      int source_port_step = 0;
#endif
      //      std::string channel = Console::GetDefaultChannel();
      //      int header_size = Console::GetDefaultHeaderSize();

      // TODO: strpe option should expect a list of arguments, not bool
      desc.add_options()
        ("help,h", "Produce this help message and exits.")
	//	("channel", boost::program_options::value<std::string>()->default_value(channel), "Name of the channel served by the streaming source.")
	/*#if defined __DBS__ || defined __ACS__
        ("enable_chunk_loss", boost::program_options::value<std::string>(), "Forces a lost of chunks.")
	#endif*/
	//("header_size", boost::program_options::value<int>()->default_value(header_size), "Size of the header of the stream in chunks.")
#if not defined __IMS__
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
	//#if defined __monitor__
	//        ("monitor", "The peer is a monitor peer (and will send to the splitter complains about lost chunks)")
	//#endif
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
    //peer.reset(new p2psp::Console_IMS());
#endif
    
#if defined __DBS__ || defined __ACS__
#if defined __monitor__
    std::cout << "Using Monitor_DBS" << std::endl;
#else
    std::cout << "Using Peer_DBS" << std::endl;
#endif /* __monitor__ */
#endif /* __DBS__ || __ACS__ */

#if defined __LRS__
    std::cout << "Using Monitor_LRS" << std::endl;
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
    //std::shared_ptr<Console> peer;
    //= new p2psp::Console();

#ifdef _1_
    //std::shared_ptr<p2psp::Console_DBS> ptr = std::static_pointer_cast<p2psp::Console_DBS>(peer);
    //class Console_DBS* peer = new Console_DBS();
    if (vm.count("monitor")) {
      LOG("Monitor peer.");
      //class Console_Monitor: public Console, public Monitor_DBS {};
      //peer->reset(new p2psp::Console_Monitor_DBS());
      //p2psp::Console_Monitor_DBS* ptr = new p2psp::Console_Monitor_DBS();
      //console.reset(ptr);
    } else {
      //peer.reset(new p2psp::Console_DBS());
      //p2psp::Console_DBS* ptr = new p2psp::Console_DBS();
      //console.reset(ptr);
      std::cout << "hola" << std::endl;
      std::cout << "SendtoCounter = " << peer->GetSendtoCounter() << std::endl;
      peer->SetSendtoCounter(10);
      std::cout << "SendtoCounter = " << peer->GetSendtoCounter() << std::endl;
    }

#endif /* _1_ */
    
    // }}}
    
    peer->Init();
    
    if (vm.count("player_port")) {
      // {{{

      peer->SetPlayerPort(vm["player_port"].as<uint16_t>());
      LOG("Player port = "
	  << peer->GetPlayerPort());

      // }}}
    }

    peer->WaitForThePlayer();
    TRACE("Player connected");

    if (vm.count("splitter_addr")) {
      // {{{

      peer->SetSplitterAddr(ip::address::from_string(vm["splitter_addr"].as<std::string>()));
      LOG("Splitter address = "
	  << peer->GetSplitterAddr());

      // }}}
    }

    if (vm.count("splitter_port")) {
      // {{{
      
      peer->SetSplitterPort(vm["splitter_port"].as<uint16_t>());
      LOG("Splitter port = "
	  << peer->GetSplitterPort());
      
      // }}}
    }
    
    peer->ConnectToTheSplitter();
    LOG("Connected to the splitter");
    /*std::cout
      << "Real splitter port = "
      << peer->GetRealSplitterPort()
      << std::endl;*/

    peer->ReceiveSourceEndpoint();
    LOG("Source = ("
	<< peer->GetSourceAddr()
	<< ","
	<< std::to_string(peer->GetSourcePort())
	<< ")");

    peer->ConnectToTheSource();
    LOG("Connected to the source");

    peer->ReceiveChannel();
    LOG("channel = "
	<< peer->GetChannel());
    
    peer->ReceiveHeaderSize();
    LOG("Header size = "
	<< peer->GetHeaderSize());

    peer->RequestHeader();
    LOG("Header requested");

    std::cout << "Relaying header ... " << std::flush;
    peer->RelayHeader();
    std::cout << "done" << std::endl;
    
    peer->ReceiveChunkSize();
    LOG("Chunk size = "
	<< peer->GetChunkSize());
    
    peer->ReceiveBufferSize();
    LOG("Buffer size = "
	<< peer->GetBufferSize());

#if defined __IMS__
    // {{{
    
    peer->ReceiveMcastGroup();
    LOG("Using IP multicast group = ("
	<< peer->GetMcastAddr().to_string()
	<< ","
	<< peer->GetMcastPort()
	<< ")");

    // }}}

#else /* __IMS__ */
    
    // {{{

    if (vm.count("max_chunk_debt")) {
      // {{{
      
      peer->SetMaxChunkDebt(vm["max_chunk_debt"].as<int>());
      LOG("Maximum chunk debt = "
	  << peer->GetMaxChunkDebt());
      //peer.reset(peer_dbs);
      
      // }}}
    }

    if (vm.count("team_port")) {
      // {{{
      
      peer->SetTeamPort(vm["team_port"].as<uint16_t>());
      LOG("team_port = "
	  << peer->GetTeamPort());
      
      // }}}
    }

    if (vm.count("use_localhost")) {
      // {{{
      
      peer->SetUseLocalHost(true);
      LOG("use_localhost = "
	  << peer->GetUseLocalHost());
      
      // }}}
    }

    // }}}
#endif /* __IMS__*/
    
#if defined __NTS__
    // {{{

    if (vm.count("source_port_step")) {
      peer->SetPortStep(vm["source_port_step"].as<int>());
    }
    LOG("Source port step ="
	<< peer->GetPortStep());

    // }}}
#endif

    peer->ListenToTheTeam();
    TRACE("Listening to the team");

#if not defined __IMS__
    // {{{

    std::cout << "Receiving the list of peers ... " << std::flush;
    peer->ReceiveTheListOfPeers();
    std::cout << "done" << std::endl;
    TRACE("List of peers received");
    LOG("Number of peers in the team (excluding me) ="
	<< std::to_string(peer/*ptr*/->GetNumberOfPeers()));    
    
    // }}}
#endif    

    peer->DisconnectFromTheSplitter();
    LOG("Recived the configuration from the splitter.");
    LOG("Clossing the connection");

    std::cout << "Buffering ... " << std::flush;
    peer->BufferData();
    std::cout << "done" << std::endl;

    peer->Start();
    LOG("Peer running in a thread");

    std::cout << _RESET_COLOR();

#if defined __IMS__
    
    std::cout << "                     | Received |     Sent |" << std::endl;
    std::cout << "                Time |   (kbps) |   (kbps) |" << std::endl;
    std::cout << "---------------------+----------+----------+" << std::endl;

#else

    /*std::cout << "+-----------------------------------------------------+" << std::endl;
    std::cout << "| Received = Received kbps, including retransmissions |" << std::endl;
    std::cout << "|     Sent = Sent kbps                                |" << std::endl;
    std::cout << "|       (Expected values are between parenthesis)     |" << std::endl;
    std::cout << "------------------------------------------------------+" << std::endl;*/
    std::cout << std::endl;
    std::cout << "                     | Received Expected |     Sent Expected | Team |" << std::endl;
    std::cout << "                Time |   (kbps)   (kbps) |   (kbps)   (kbps) | size | Peer list" << std::endl;
    std::cout << "---------------------+-------------------+-------------------+------+----------..." << std::endl;

#endif
    
    //float kbps_recvfrom = 0.0f;
    //float kbps_sendto = 0.0f;
    int kbps_recvfrom = 0;
    int kbps_sendto = 0;
    int last_sendto_counter = -1;
    int last_recvfrom_counter = peer->GetRecvfromCounter();
    
#if not defined __IMS__

    int last_chunk_number = peer->GetPlayedChunk();
    //float kbps_expected_recv = 0.0f;
    int kbps_expected_recv = 0;
    if (peer->GetSendtoCounter() < 0) {
      last_sendto_counter = 0;
    } else {
      //peer->SetSendtoCounter(0);
      last_sendto_counter = 0;
    }
    float team_ratio = 0.0f;
    //int team_ratio = 0f;
    //float kbps_expected_sent = 0.0f;
    int kbps_expected_sent = 0;
    // float nice = 0.0f;
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
	//std::shared_ptr<p2psp::Console_DBS> ptr = std::static_pointer_cast<p2psp::Console_DBS>(peer);
	team_ratio = /*ptr*/peer->GetPeerList()->size() / (/*ptr*/peer->GetPeerList()->size() + 1.0f);
      }
      kbps_expected_sent = (int)(kbps_expected_recv * team_ratio);

      /*
	if (kbps_recvfrom > 0 and kbps_expected_recv > 0) {
	// nice = 100.0 / (kbps_expected_recv / kbps_recvfrom) *
	// (peer->GetPeerList()->size() + 1.0f);
	} else {
	// nice = 0.0f;
	}*/

      if (kbps_expected_recv < kbps_recvfrom) {
	std::cout <<_SET_COLOR(_GREEN);
      } else if (kbps_expected_recv > kbps_recvfrom) {
	std::cout << _SET_COLOR(_RED);
      }

#endif /* not defined __IMS__ */
    
      std::cout
	<< " |";
      std::cout
	<< std::setw(9)
	<< kbps_recvfrom
	<< _RESET_COLOR();

#if not defined __IMS__
    
      std::cout
	<< std::setw(9)
	<< kbps_expected_recv;

      if (kbps_expected_sent < kbps_sendto) {
	std::cout <<_SET_COLOR(_GREEN);
      } else if (kbps_expected_sent > kbps_sendto) {
	std::cout << _SET_COLOR(_RED);
      }

#endif /* not defined __IMS__ */
    
      std::cout
	<< " |";
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
    
      // TODO: Format default options
      //boost::format format("Defaut = %5i");
    
      // TODO: format
      //O(kbps_expected_recv);
      //O(kbps_recvfrom);
      //#print(("{:.1f}".format(nice)).rjust(6), end=' | ')
      //#sys.stdout.write(Color.none)
#ifndef __IMS__

      /*if (kbps_expected_sent > kbps_sendto) {
	std::cout << _SET_COLOR(_RED);
	} else if (kbps_expected_sent < kbps_sendto) {
	std::cout << _SET_COLOR(_GREEN);
	}*/

      // TODO: format
      //std::cout << kbps_sendto;
      //std::cout << kbps_expected_sent;
      // sys.stdout.write(Color.none)
      // print(repr(nice).ljust(1)[:6], end=' ')
      {
	//std::shared_ptr<p2psp::Console_DBS> ptr = std::static_pointer_cast<p2psp::Console_DBS>(peer);
	std::cout
	  << std::setw(5)
	  << /*ptr*/peer->GetPeerList()->size()
	<< " |";
	counter = 0;
	for (std::vector<boost::asio::ip::udp::endpoint>::iterator p = /*ptr*/peer->GetPeerList()->begin();
	     p != /*ptr*/peer->GetPeerList()->end();
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
  //try {
    return p2psp::run(argc, argv);
    //} catch (boost::system::system_error e) {
    //TRACE(e.what());
    //}

  return -1;

}
