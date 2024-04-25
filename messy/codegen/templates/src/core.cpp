#include <core.hpp>

int64_t get_resolution_val(sc_core::sc_time_unit time_unit){
    switch (time_unit)
    {
    case sc_core::SC_PS:
        return 1;
    case sc_core::SC_NS:
        return 1000;
    case sc_core::SC_US:
        return 1000000;
    case sc_core::SC_MS:
        return 1000000000;
    case sc_core::SC_SEC:
        return 1000000000000;
    default:
        return 1;
    }
}

void Core::run()
{
    iss_adapter->startup();
    
    while(!this->iss_adapter->finished)
        continue_messy(true);

    this->close();
}

void Core::run_next_sc(){
    wait(next_timestamp-sc_time_stamp().to_double(),sc_core::SC_PS);
    sc_timestamp=double(sc_time_stamp().to_double());
}

void Core::close(){
    // wait until next resolutional value(next ms)
    wait(get_resolution_val(${resolution})-(next_timestamp%get_resolution_val(${resolution})),sc_core::SC_PS);
    delete_n_requests(request_queue_size());
    this->iss_adapter->close();
    sc_stop();
}

void Core::handle_req_queue(){
    int core_requests_size=request_queue_size();
    for(int i=0;i<core_requests_size;i++)   handle_req(get_request_at(i));
    delete_n_requests(core_requests_size);
}

void Core::continue_messy(bool handle_req_queue){

    next_timestamp = this->iss_adapter->exec_events_at(sc_timestamp);

    this->tot_power += this->iss_adapter->get_power_at(sc_timestamp);

    simulation_iters++;

    if(handle_req_queue)    this->handle_req_queue();
        
    this->run_next_sc();

}

void Core::handle_req(MessyRequest *req)
{
    //Here connect the signals with SystemC simulator signals   
    //std::cout << sc_time_stamp().to_double() << std::endl;
    if (req->read_req)
    {
        functional_bus_flag.write(true);
        request_ready.write(true);
        
        request_address.write(req->addr);
        request_size.write(req->size);
        wait();
        // segmentation fault
        if(idx_sensor.read()<0) this->close();
        request_ready.write(false);
        wait();

        // read onto iss data
        uint8_t* sensor_memory=(uint8_t*)request_value.read();
        for(unsigned int i=0;i<req->size;i++)    ((uint8_t*)req->data)[i] = sensor_memory[i];
    }
    else
    {
        request_data.write(((uint8_t*)req->data));
        functional_bus_flag.write(false);
        request_ready.write(true);

        request_address.write(req->addr);
        request_size.write(req->size);
        wait();
        if(idx_sensor.read()<0) this->close();
        request_ready.write(false);
        wait();
        
    }
    iss_adapter->custom_reply(req);   
}


void Core::request_delay(double start_time,int time_to_skip,int resolution){
    double time=(time_to_skip*get_resolution_val(${resolution}))+start_time;
    while(next_timestamp<time && !this->iss_adapter->finished)
        this->continue_messy(false);
}

void Core::grant_req(MessyRequest *req)
{

}

void Core::reply_to_req(MessyRequest *req)
{

}