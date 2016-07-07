//
//  splitter.cc -- Console version of a P2PSP peer
//
//  This code is distributed under the GNU General Public License (see
//  THE_GENERAL_GNU_PUBLIC_LICENSE.txt for extending this information).
//
//  Copyright (C) 2016, the P2PSP team.
//
//  http://www.p2psp.org
//

#include <iostream>
#include <memory>
#include "common.h"
#include "core/common.h"
#if defined __IMS__
#include "core/splitter_ims.h"
#elif defined __DBS__
#include "core/splitter_dbs.h"
#endif
//#include "core/splitter_acs.h"
//#include "core/splitter_lrs.h"
//#include "core/splitter_nts.h"
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <signal.h>

#if defined __IMS__
p2psp::Splitter_IMS splitter;
#elif defined __DBS__
p2psp::Splitter_DBS splitter;
#endif

void HandlerCtrlC(int s) {
  LOG("Keyboard interrupt detected ... Exiting!");

  // Say to daemon threads that the work has been finished,
  splitter.SetAlive(false);
}

void HandlerEndOfExecution() {
  // Wake up the "moderate_the_team" daemon, which is waiting in a recvfrom().
  //splitter_ptr->SayGoodbye();

  // Wake up the "handle_arrivals" daemon, which is waiting in an accept().
  boost::asio::io_service io_service_;
  boost::system::error_code ec;
  boost::asio::ip::tcp::socket socket(io_service_);
  boost::asio::ip::tcp::endpoint endpoint(
      boost::asio::ip::address::from_string("127.0.0.1"),
      splitter.GetSplitterPort());

  socket.connect(endpoint, ec);

  // TODO: If "broken pipe" errors have to be avoided, replace the close method
  // with multiple receive calls in order to read the configuration sent by the
  // splitter
  socket.close();
}

/*bool HasParameter(const boost::program_options::variables_map& vm,
		  const std::string& param_name,
		  char min_magic_flags) {
  if (!vm.count(param_name)) {
    return false;
  }
  return true;
  }*/

int main(int argc, const char *argv[]) {

  // {{{ Argument parsing

  const char description[80] = "This is the splitter node of a P2PSP team.\n"
#if defined __IMS__
      "Using IMS.\n"
#elif defined __DBS__
      "Using DBS.\n"
#endif
         "Parameters";
  
  boost::program_options::options_description desc(description);

  //~ {

    int buffer_size = splitter.GetDefaultBufferSize();
    std::string channel = splitter.GetDefaultChannel();
    int chunk_size = splitter.GetDefaultChunkSize();
    int splitter_port = splitter.GetDefaultSplitterPort();
    std::string source_addr = splitter.GetDefaultSourceAddr();
    int source_port = splitter.GetDefaultSourcePort();
    int header_length = splitter.GetDefaultHeaderLength();
#if defined __IMS__
    std::string mcast_addr = splitter.GetDefaultMcastAddr();
    int TTL = splitter.GetDefaultTTL();
#elif defined __DBS__
    int max_number_of_chunk_loss = splitter.GetDefaultMaxNumberOfChunkLoss();
    int max_number_of_monitors = splitterGetDefaultMaxNumberOfMonitors();
#endif

    // TODO: strpe option should expect a list of arguments, not bool
    desc.add_options()
      ("help,h", "Produces this help message and exits.")
      ("buffer_size", boost::program_options::value<int>()->default_value(buffer_size), "Length of the buffer in chunks.")
      ("channel", boost::program_options::value<std::string>()->default_value(channel), "Name of the channel served by the streaming source.")
      ("chunk_size", boost::program_options::value<int>()->default_value(chunk_size), "Chunk size in bytes.")
      ("header_length", boost::program_options::value<int>()->default_value(header_length), "Size of the header of the stream in bytes.")
#if defined __DBS__
      ("max_number_of_chunk_loss", boost::program_options::value<int>()->default_value(max_number_of_chunk_loss), "Maximum number of lost chunks for an unsupportive peer.")
      ("max_number_of_monitors", boost::program_options::value<int>()->default_value(max_number_of_monitors), "Maximum number of monitors in the team. The first connecting peers will automatically become monitors.")
#endif
#if defined __IMS__
      ("mcast_addr",boost::program_options::value<std::string>()->default_value(mcast_addr), "IP multicast address used to serve the chunks.")
#endif
      ("source_addr", boost::program_options::value<std::string>()->default_value(source_addr), "IP address or hostname of the streaming server.")
      ("source_port", boost::program_options::value<int>()->default_value(source_port), "Port where the streaming server is listening.")
#if defined __IMS__
      ("TTL", boost::program_options::value<int>()->default_value(TTL), "Time To Live of the multicast messages.")
#endif
      ("splitter_port", boost::program_options::value<int>()->default_value(splitter_port), "Port to serve the peers.");

#ifdef _1_
      (
       "IMS", "Uses the IP multicast infrastructure, if available. IMS mode is incompatible with ACS, LRS, DIS and NTS modes.")
      (
       "NTS", "Enables NAT traversal.")
      (
       "ACS", "Enables Adaptative Chunk-rate.")
      (
       "LRS", "Enables Lost chunk Recovery")
      (
       "DIS", "Enables Data Integrity check.")
      (
       "strpe", "Selects STrPe model for DIS.")
      (
       "strpeds", "Selects STrPe-DS model for DIS.")
      (
       "strpeds_majority_decision", "Sets majority decision ratio for STrPe-DS model.")
      (
       "strpe_log", boost::program_options::value<std::string>(),
       "Loggin STrPe & STrPe-DS specific data to file.")
#endif

  boost::program_options::variables_map vm;
  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  } catch (std::exception &e) {

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


  if (vm.count("buffer_size")) {
    splitter.SetBufferSize(vm["buffer_size"].as<int>());
    TRACE("Buffer size = "
	  << splitter.GetBufferSize());
  }

  if (vm.count("channel")) {
    splitter.SetChannel(vm["channel"].as<std::string>());
    TRACE("Channel = "
	  << splitter.GetChannel());
  }

  if (vm.count("chunk_size")) {
    splitter.SetChunkSize(vm["chunk_size"].as<int>());
    TRACE("Chunk size = "
	  << splitter.GetChunkSize());
  }

  if (vm.count("splitter_port")) {
    splitter.SetTeamPort(vm["splitter_port"].as<int>());
    TRACE("Splitter port = "
	  << splitter.GetSplitterPort());
  }

  if (vm.count("source_addr")) {
    splitter.SetSourceAddr(vm["source_addr"].as<std::string>());
    TRACE("Source address = "
	  << splitter.GetSourceAddr());
  }

  if (vm.count("source_port")) {
    splitter.SetSourcePort(vm["source_port"].as<int>());
    TRACE("Source port = "
	  << splitter.GetSourcePort());
  }

#if defined __DBS__
  
  if (vm.count("max_number_of_chunk_loss")) {
    splitter.SetMaxNumberOfChunkLoss(vm["max_number_of_chunk_loss"].as<int>());
    TRACE("Maximun number of lost chunks ="
	  << splitter.GetMaxNumberOfChunkLoss());
  }

  if (vm.count("max_number_of_monitors")) {
    splitter_dbs->SetMaxNumberOfMonitors(vm["max_number_of_monitors"].as<int>());
    TRACE("Maximun number of monitors = "
	  << splitter.GetMaxNumberOfMonitors());
  }

#endif

  splitter.Start();


  LOG("         | Received  | Sent      | Number       losses/ losses");
  LOG("    Time | (kbps)    | (kbps)    | peers (peer) sents   threshold period kbps");
  LOG("---------+-----------+-----------+-----------------------------------...");

  int last_sendto_counter = splitter.GetSendToCounter();
  int last_recvfrom_counter = splitter.GetRecvFromCounter();

  int chunks_sendto = 0;
  int kbps_sendto = 0;
  int kbps_recvfrom = 0;
  int chunks_recvfrom = 0;
#if defined __DBS__
  std::vector<boost::asio::ip::udp::endpoint> peer_list;
#endif
  
  // Listen to Ctrl-C interruption
  struct sigaction sigIntHandler;
  sigIntHandler.sa_handler = HandlerCtrlC;
  sigemptyset(&sigIntHandler.sa_mask);
  sigIntHandler.sa_flags = 0;
  sigaction(SIGINT, &sigIntHandler, NULL);

  while (splitter.isAlive()) {
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    chunks_sendto = splitter.GetSendToCounter() - last_sendto_counter;
    kbps_sendto = (chunks_sendto * splitter.GetChunkSize() * 8) / 1000;
    chunks_recvfrom = splitter.GetRecvFromCounter() - last_recvfrom_counter;
    kbps_recvfrom = (chunks_recvfrom * splitter.GetChunkSize() * 8) / 1000;
    last_sendto_counter = splitter.GetSendToCounter();
    last_recvfrom_counter = splitter.GetRecvFromCounter();


    
    LOG("|" << kbps_recvfrom << "|" << kbps_sendto << "|");
    // LOG(_SET_COLOR(_CYAN));
#if defined __DBS__
    peer_list = splitter_dbs.GetPeerList();
    LOG("Size peer list: " << peer_list.size());
    
    if (peer_list.size()>0){
      std::vector<boost::asio::ip::udp::endpoint>::iterator it;
      for (it = peer_list.begin(); it != peer_list.end(); ++it) {
	// _SET_COLOR(_BLUE);
	LOG("Peer: " << *it);
	// _SET_COLOR(_RED);
	
	LOG(splitter.GetLoss(*it)
	    << "/"
	    << chunks_sendto
	    << " "
	    << splitter.GetMaxNumberOfChunkLoss());
	
	/*if (splitter_dbs->GetMagicFlags() >= p2psp::Common::kACS) { // If is ACS
	// _SET_COLOR(_YELLOW);
	LOG(splitter_acs->GetPeriod(*it));
	// _SET_COLOR(_PURPLE)
	LOG((splitter_acs->GetNumberOfSentChunksPerPeer(*it) *
	splitter_acs->GetChunkSize() * 8) /
	1000);
	splitter_acs->SetNumberOfSentChunksPerPeer(*it, 0);
	}*/
      }
    }
#endif     
  }

  LOG("Ending");
  HandlerEndOfExecution();

  return 0;
}
