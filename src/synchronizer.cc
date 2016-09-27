/*
 synchronizer.cc
 P2PSP

 This code is distributed under the GNU General Public License (see
 THE_GENERAL_GNU_PUBLIC_LICENSE.txt for extending this information).
 Copyright (C) 2016, the P2PSP team.
 http://www.p2psp.org
*/

#include "synchronizer.h"

namespace p2psp {
    Synchronizer::Synchronizer()
    : io_service_(),
    player_socket_(io_service_),
    acceptor_(io_service_)
    {
      player_port = 15000;
      peer_data = std::vector<std::vector<char> >();
      synchronized=false;
      buffered = false;
    }

    Synchronizer::~Synchronizer()
    {}


    void Synchronizer::Run(int argc, const char* argv[]) throw(boost::system::system_error)
    {
      boost::program_options::options_description desc("This is the synchronizer node of P2PSP.\n");
      desc.add_options()
      ("help", "Produce this help message and exit.")
      ("peers",boost::program_options::value<std::vector<std::string> >()-> multitoken(),"Peers list");
      boost::program_options::variables_map vm;
      try{
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
      if(argc < 2)
      throw std::exception();
      }
      catch(std::exception& e)
      {
        // If the argument passed is unknown, print the list of available arguments
        std::cout<<desc<<std::endl;
      }
      boost::program_options::notify(vm);
      if(vm.count("help"))
      {
        std::cout<< desc <<std::endl;
      }
      if(vm.count("peers"))
      {
        peer_list = &vm["peers"].as<std::vector<std::string> >();
        std::cout<<peer_list->size()<< " peers passed to the synchronizer\n";
        peer_data.resize(peer_list->size());
        // Run the RunThreads function which in turn starts the threads which connect to the peers
        boost::thread t1(&Synchronizer::RunThreads,this);
        t1.join();
      }
    }

    void Synchronizer::RunThreads()
    {
      // Iterarate through the peer_list and start a thread to connect to the peer for every peer in peer_list
      for(std::vector<std::string>::const_iterator it = peer_list->begin();it!=peer_list->end();++it)
      {
        std::cout<<"Running thread " << it-peer_list->begin()+1 << " of " << peer_list->end() - peer_list->begin()<<"\n";
        thread_group_.interrupt_all();
        thread_group_.add_thread(new boost::thread(&Synchronizer::ConnectToPeers,this,*it,(it-peer_list->begin())));

      }
      boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
      WaitForThePlayer();
      mixed_data.resize(set_buffer_size);
      //thread_group_.add_thread(new boost::thread(&Synchronizer::PlayInitChunks,this));
      boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
      Synchronize();
      //thread_group_.add_thread(new boost::thread(&Synchronizer::MixStreams,this));
      boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
      //InitBuffer();
      thread_group_.add_thread(new boost::thread(&Synchronizer::PlayChunk,this));
      //thread_group_.add_thread(new boost::thread(&Synchronizer::ChangeStream,this));
      CheckPlayerStatus();
      //thread_group_.join_all(); //Wait for all threads to complete
    }

    void Synchronizer::ConnectToPeers(std::string s, int id) throw(boost::system::system_error)
    {
        std::cout<<"Connecting to " << s<<"\n";
        std::vector<std::string> fields;
        boost::algorithm::split(fields,s,boost::is_any_of(":"));
        const boost::asio::ip::address hs = boost::asio::ip::address::from_string(fields[0]);
        unsigned short port = boost::lexical_cast<unsigned short>(fields[1]);
        boost::asio::ip::tcp::endpoint peer(hs,port);
        boost::asio::ip::tcp::socket peer_socket (io_service_);
        peer_socket.connect(peer);
        std::cout<<"Connected to "<< s<<"\n";
        peer_data[id].resize(1024*1024);
        std::vector<char> message(1024),empty_chunk(100,0);
        std::string empty_chunk_str (empty_chunk.begin(),empty_chunk.end());
        std::vector<char>::iterator pdpos=peer_data[id].begin();
        std::cout<<"Resized the vector\n";
        while(1)
        {
        //TRACE("Receiving data from "<< s);
        //mtx.lock();
        size_t bytes = boost::asio::read(peer_socket,boost::asio::buffer(message,1024));
        //TRACE("Message size "<<bytes);
        std::string empty_chunk_test (message.begin(),message.end());
        size_t found = empty_chunk_test.find(empty_chunk_str);
        if(found!=std::string::npos)
        {
          //peer_id=(peer_id+1)%peer_list->size();
          std::cout<<"Changing streams\n";
          ChangeStream();
          continue;
        }
        std::copy(message.begin(),message.end(),pdpos);
        message = std::vector<char> (1024);
        //peer_data[id].insert(pdpos,message.begin(),message.end());
        //mtx.unlock();
        //peer_data[id].resize(peer_data[id].size()+1024);
        if(synchronized)
        {
          //std::vector<char> v (pdpos,pdpos+1024);
          if(pdpos-peer_data[id].begin()<0)
          pdpos=peer_data[id].begin();
          if(id!=peer_id){
          //peer_data[id].erase(peer_data[id].begin(),peer_data[id].begin()+1024);
          continue;
          }
          mtx2.lock();
          mixed_data[chunk_added%set_buffer_size] = std::vector<char>(pdpos,pdpos+1024); //Add 1024 bytes of each peer chunk to the set
          chunk_added++;
          std::cout<<"Chunk "<<chunk_added<<" added at "<<(pdpos-peer_data[id].begin())/1024<<"\n";
          mtx2.unlock();
          //peer_data[id].erase(peer_data[id].begin(),peer_data[id].begin()+1024);
        }
        pdpos=(pdpos+1024);
        if(pdpos-peer_data[id].begin()>1024*1024)
        pdpos=peer_data[id].begin();
        }

    }

    void Synchronizer::Synchronize()
    {
        /*Here we start with a search string and keep on increasing its length until we find a constant offset from the haystack
          string. Once we find the offset, we trim the corresponding peer_data vector according to the offset, so that we achieve
          synchronization. Synchronization is a one time process.
        */
        std::cout<<"Attempting to synchronize peers\n";
        int start_offset=100,offset=6;
        //mtx.lock();
        //mtx.unlock();
        std::string needle(peer_data[0].begin(),peer_data[0].begin()+offset);
        for(std::vector<std::vector<char> >::iterator it = peer_data.begin()+1; it!=peer_data.end();++it) //Iterating through all the elements of peer_data vector
        {
            std::string haystack (it->begin(),it->end());
            std::cout<<"Haystack size "<< haystack.size()<<"\n";
            std::size_t found,found_before;
            while((found=haystack.find(needle))!=std::string::npos && found!=found_before)
            {
                offset++;
                needle = std::string(peer_data[0].begin()+start_offset,peer_data[0].begin()+start_offset+offset); //Incremental length of the search string
                found_before=found; //We stop the loop when the found variable no more changes
            }
            if(found == std::string::npos) //If the string matching fails, continue
            {
            std::cout<<"Synchronization of peer "<< it-peer_data.begin()<<" with peer 0 failed\nIgnoring the peer\n";
            Synchronize();
            return;
            }
            std::cout<<"Synchronized peer " << it-peer_data.begin()<<"at "<<found<<"\n";
            it->erase(it->begin(),it->begin()+found); //Trim the first 'found' bytes of the vector
            it->resize(1024*1024);
        }
        peer_data[0].erase(peer_data[0].begin(),peer_data[0].begin()+start_offset+offset-1);
        peer_data[0].resize(1024*1024);
        synchronized=true;
    }

    void Synchronizer::PlayChunk()
    {
      try{
        while(FindNextChunk())
        {
        std::cout<<"Writing to the player from stream "<<peer_id<<" | Chunk "<<chunk_removed<<" | "<<mixed_data[chunk_removed%set_buffer_size].size()<<"\n";
        //mtx2.lock();
        boost::asio::write(player_socket_,boost::asio::buffer(mixed_data[chunk_removed%set_buffer_size]));
        chunk_removed++;
        //mtx2.unlock();
        //TRACE("Chunk "<<chunk_removed<<" removed");
        }
      }
      catch(boost::system::system_error& e)
      {
        std::cout<<e.what()<<"\n";
        player_alive=false;
      }
      }


    bool Synchronizer::FindNextChunk()
    {
      while(chunk_added<=chunk_removed)
      MixStreams();
      return true;
    }

    void Synchronizer::MixStreams() throw(boost::system::system_error)
    {
      //while(1)
      //{
        //mtx2.lock();
        for(;(chunk_added-chunk_removed)<=1000;);
        //mtx2.unlock();
      //}

    }

    void Synchronizer::WaitForThePlayer()
    {
      boost::asio::ip::tcp::endpoint player_endpoint (boost::asio::ip::tcp::v4(), player_port);
      acceptor_.open(player_endpoint.protocol());
      acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
      acceptor_.bind(player_endpoint);
      acceptor_.listen();
      std::cout<<"Waiting for the player at (" << player_endpoint.address().to_string() << ","
            << std::to_string(player_endpoint.port())
            << ")\n";
      acceptor_.accept(player_socket_);
      std::cout<<"The player is ("
            << player_socket_.remote_endpoint().address().to_string() << ","
            << std::to_string(player_socket_.remote_endpoint().port()) << ")\n";
      std::string s = "HTTP/1.1 200 OK\r\n\r\n";
      boost::asio::write(player_socket_,boost::asio::buffer(&s[0],s.size()));
      player_alive=true;
    }

    void Synchronizer::ChangeStream()
    {
      boost::this_thread::sleep(boost::posix_time::seconds(20));
      peer_id=(++peer_id)%(peer_list->size());
      std::cout<<"Changed Stream\n";
    }

    void Synchronizer::CheckPlayerStatus()
    {
    	while(player_alive);
    	return;
    }
}

int main(int argc, const char* argv[])
{
  try {
  p2psp::Synchronizer syn;
  syn.Run(argc, argv);
  }
  catch(boost::system::system_error e)
  {
    std::cout<<e.what();
  }
  return -1;
}
