//
//  main.cpp
//  EventSimulation
//
//  Created by Alexander Sloan on 3/30/19.
//  Copyright © 2019 Alexander Sloan. All rights reserved.
//

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <array>
#include <unordered_map>
#include <iomanip>


using namespace std;
#define NUM_TELLERS 6
#define SIM_LENGTH 43200

typedef struct {
  int customerID;
  int eventTime;
  int duration;
  int event;
  int totalServiceTime;
  int cashier;
  
} Event;

// define event types
typedef enum {ARRIVAL, DEPARTURE} EventType;

// how to compare event times for the priority queue
struct compareEventTime
{
  bool operator()(const Event& lhs, const Event& rhs) const
  {
    return lhs.eventTime > rhs.eventTime;
  }
};

// Bank Simulation Class
class BankSim
{
private:
  // event queue -- priority queue with events queued by event time
  priority_queue<Event,vector<Event>,compareEventTime> eventQueue;
  // wait queue -- customers waiting in line
  queue<Event> bankQueue;
  int currentTime = 0;
  bool debugOn = false;
  int tellersAvailable = NUM_TELLERS;
public:
  void addEvent(Event & event);
  void addBankQueueCustomer(Event & event);
  void setArrivalEvent(Event &event);
  void setCustomerDeparture(Event &nextCustomer, Event &nextEvent);
  void runSim(vector<int> &serviceTimes);
  long getBankQueueSize(){
    return eventQueue.size();
  }
};

// add a priority queue event
void BankSim::addEvent(Event & event)
{
  eventQueue.push(event);
}

// add a bank queue customer
void BankSim::addBankQueueCustomer(Event & event)
{
  bankQueue.push(event);
}

void BankSim::setArrivalEvent(Event &event)
{
  event.totalServiceTime = event.duration;
  event.eventTime = currentTime + event.duration;
  event.event = DEPARTURE;
}

void BankSim::setCustomerDeparture(Event &nextCustomer, Event &nextEvent)
{
  nextCustomer.eventTime = currentTime + nextEvent.duration;
  nextCustomer.event = DEPARTURE;
}


// run the bank simulation
void BankSim::runSim(vector<int> &serviceTime)
{
  while (!eventQueue.empty()) {
    Event nextEvent = eventQueue.top(); // get next event in priority queue
    currentTime = nextEvent.eventTime;
    //kick customers out
    if(currentTime > SIM_LENGTH){
      break;
    }
    eventQueue.pop();
    switch (nextEvent.event) {
      case ARRIVAL:
        if (tellersAvailable) {
          setArrivalEvent(nextEvent);
          addEvent(nextEvent);
          tellersAvailable--;
        } else {
          // no tellers available, put customer in bank queue
          addBankQueueCustomer(nextEvent);
        }
        break;
      case DEPARTURE:
        if(!bankQueue.empty() ) {
          Event nextCustomer = bankQueue.front();
          bankQueue.pop();
          nextCustomer.totalServiceTime = currentTime - nextCustomer.eventTime + nextCustomer.duration;
          //push back the total service time
          serviceTime.push_back(nextEvent.totalServiceTime);
          setCustomerDeparture(nextCustomer, nextEvent);
          addEvent(nextCustomer);
        } else {
          //push back the total service time
          serviceTime.push_back(nextEvent.totalServiceTime);
          tellersAvailable++;
        }
        break;
      default:
        cout << "ERROR: Should never get here! " << endl;
    }
  }
}


//grocery store simulation class
class GrocerySim
{
private:
  // event queue -- priority queue with events queued by event time
  priority_queue<Event,vector<Event>,compareEventTime> eventQueue;
  array<int,6> cashiers;
  int currentTime = 0;
public:
  void addEvent(Event event);
  void runSim(vector<int> &serviceTimes);
  void setArrivalEvent(Event &nextEvent,int shortestLine);
  long getGroceryQueueSize(){
    return eventQueue.size();
  }
  int chooseShortestLine (array<int,6> &lines);
};

int GrocerySim::chooseShortestLine(array<int, 6> &lines){
  int minIndex = 0;
  
  for(int i=1; i<lines.size(); i++){
    if(lines.at(minIndex) > lines.at(i)){
      minIndex = i;
    }
  }
  return minIndex;
}


void GrocerySim::addEvent(Event event)
{
  eventQueue.push(event);
}

void GrocerySim::setArrivalEvent(Event &nextEvent,int shortestLine) {
  nextEvent.cashier = shortestLine;
  nextEvent.totalServiceTime = cashiers.at(shortestLine) + nextEvent.duration;
  cashiers.at(shortestLine)+=nextEvent.duration;
  nextEvent.eventTime = currentTime + cashiers.at(shortestLine);
  nextEvent.event = DEPARTURE;
}

//Grocery simulation
void GrocerySim::runSim(vector<int> &serviceTime)
{
  while (!eventQueue.empty()) {
    Event nextEvent = eventQueue.top(); // get next event in priority queue
    currentTime = nextEvent.eventTime;
    
    //kick customers out
    if(currentTime > SIM_LENGTH){
      break;
    }
    
    eventQueue.pop();
    int shortestLine;
    
    switch (nextEvent.event) {
      case ARRIVAL:
        //picking the index of the shortest line
        shortestLine = chooseShortestLine(cashiers);
        setArrivalEvent(nextEvent, shortestLine);
        addEvent(nextEvent);
        
        break;
      case DEPARTURE:
        cashiers.at(nextEvent.cashier) -= nextEvent.duration;
        serviceTime.push_back(nextEvent.totalServiceTime);
        break;
      default:
        cout << "ERROR: Should never get here! " << endl;
    }
  }
}

double printPercentiles (vector<int> &serviceTimes, string name){
  sort(serviceTimes.begin(),serviceTimes.end());
  
  int service10thIndex = .1*serviceTimes.size();
  int service50thIndex = .5*serviceTimes.size();
  int service90thIndex = .9*serviceTimes.size();
  
  
  double service10th = (double) serviceTimes.at(service10thIndex)/60;
  double service50th = (double) serviceTimes.at(service50thIndex)/60;
  double service90th = (double) serviceTimes.at(service90thIndex)/60;
  
  cout<<fixed<<setprecision(2)<<name<<" service times in minutes:\n"<<" 10th %tile: "<<service10th<<"\n"<<" 50th %tile: "<<service50th<<"\n"<<" 90th %tile: "<<service90th<<"\n"<<endl;
  
  return service90th;
}

int main(int argc, const char * argv[]) {
  
  if(argc!=4){
    cerr<<"Invalid number of arguments!"<<endl;
    exit(1);
  }
  //customers/min = .32 would mean .32 customers arrive every minute
  double arrivalRate = stod(argv[1]);
  
  //max number of minutes a customer can take
  double maxCSTime = stod(argv[2]);
  //max number of seconds a customer can take
  int maxCSTimeSeconds = maxCSTime*60;
  
  //random seed
  double seed = stod(argv[3]);
  
  int totalMinutes = 12*60; //12 hours * 60 minutes
  
  int totalCustomers = arrivalRate * totalMinutes;
  
  
  
  BankSim bankSim;
  GrocerySim grocerySim;
  Event myEvent;
  srand(seed);
  
  for (int i=0; i<totalCustomers; i++) {
    myEvent.eventTime = rand()% SIM_LENGTH; //+1
    myEvent.duration = rand()% maxCSTimeSeconds; //+1
    myEvent.event = ARRIVAL;
    myEvent.customerID = i;
    bankSim.addEvent(myEvent);
    grocerySim.addEvent(myEvent);
  }
  
  //    long bankSize = bankSim.getBankQueueSize();
  //    long grocerySize = grocerySim.getGroceryQueueSize();
  
  //vectors to store the service times
  vector<int> bankServiceTimes;
  vector<int> groceryServiceTimes;
  
  bankSim.runSim(bankServiceTimes);
  grocerySim.runSim(groceryServiceTimes);
  
  
  printPercentiles(bankServiceTimes, "Bank");
  printPercentiles(groceryServiceTimes,"Supermarket");
  
  //    cout<<fixed<<setprecision(2)<<bankT<<"\t"<<storeT<<endl;
  
  return 0;
}
