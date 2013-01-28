//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "NewscastCache.h"

NewscastCache::NewscastCache(int size) : cOwnedObject(){
    m_maxEntries = size;
}

NewscastCache::~NewscastCache(){
    currentCache.clear();
}

cOwnedObject* NewscastCache::dup() const{
    NewscastCache* ret = new NewscastCache(m_maxEntries);
    CacheSet::iterator it;
    for (it = currentCache.begin(); it != currentCache.end(); it++){
        EV << "dup: " << (*it).getAddress() << endl;
        ret->setEntry( (*it).getAgent(), (*it).getAddress(), (*it).getTimestamp(), (*it).getValue() );
    }

    return ret;
}

NewscastCache NewscastCache::dup2(){
    NewscastCache ret = NewscastCache(m_maxEntries);
    CacheSet::iterator it;
    for (it = currentCache.begin(); it != currentCache.end(); it++){
        EV << "dup2: " << (*it).getAddress() << endl;
        ret.setEntry( (*it).getAgent(), (*it).getAddress(), (*it).getTimestamp(), (*it).getValue() );
    }

    return ret;
}

void NewscastCache::setEntry(std::string agent, IPvXAddress addr, simtime_t timestamp, GossipUserData* value){

    for (unsigned int i = 0; i < currentCache.size(); i++)
        if (currentCache.at(i).getAgent().compare(agent) == 0){
            currentCache.at(i).setAddress(addr);
            currentCache.at(i).setTimestamp(timestamp);
            currentCache.at(i).setValue(value);
            return;
        }

    NewscastCacheEntry entry;
        entry.setAgent(agent);
        entry.setAddress(addr);
        entry.setTimestamp(timestamp);
        entry.setValue(value);
    currentCache.push_back(entry);

    return;
    /*
    EV << "[set-entry]my name: " << agent << " addr: " << addr.str() <<  endl;
    NewscastCacheEntry* entry = findEntryForAgent(agent);
    if (entry == NULL){ // generate new entry
        entry = new NewscastCacheEntry();
        entry->setAgent(agent);
        entry->setTimestamp(0);
        currentCache.push_back(*entry);
    }

    // check timestamp ... new one < old? -> return
    if (timestamp < entry->getTimestamp()) return;

    // set entry
    entry->setAddress(addr); // should i update this everytime? hmm
    entry->setTimestamp(timestamp);
    if (value == NULL)
        entry->setValue(NULL);
    else
        entry->setValue(value->dup());

    EV << "[set-entry2]my name: " << entry->getAgent() << " addr: " << entry->getAddress().str() <<  endl;
    //EV << "[set-entry-end]my name: " << entry->getAgent() << " value: " << entry->getValue() <<  endl;
    //entry = findEntryForAgent(agent);
    //EV << "[set-entry-end2]my name: " << entry->getAgent() << " value: " << entry->getValue() <<  endl;*/
}

void NewscastCache::merge(NewscastCache* cache){
    EV << "merge: " << cache->getSize() << endl;
    // insert the entries from the new cache into the current
    CacheSet::iterator it;
    for (it = cache->currentCache.begin(); it != cache->currentCache.end(); it++){
        setEntry( (*it).getAgent(), (*it).getAddress(), (*it).getTimestamp(), (*it).getValue() );
    }

    while (currentCache.size() > m_maxEntries){  // while we have more than m_maxEntries ...
        int oldest = 0; simtime_t time = currentCache.at(0).getTimestamp();

        // find the oldest one
        for (unsigned int i = 1; i < currentCache.size(); i++)
            if (currentCache.at(i).getTimestamp() < time){
                time = currentCache.at(i).getTimestamp();
                oldest = i;
            }

        // set the pointer to the oldest one
        it = currentCache.begin();
        it += oldest;

//        printCache();
//        EV << "Deleting: " << (*it)->getAgent() << endl;

        // delete it
        //delete *it;
        currentCache.erase(it);
    }
}

NewscastCacheEntry* NewscastCache::findEntryForAgent(std::string agent){
    for (unsigned int i = 0; i < currentCache.size(); i++)
        if (currentCache.at(i).getAgent().compare(agent) == 0)
            return &currentCache.at(i);

    return NULL;
}


void NewscastCache::printCache(){
    CacheSet::iterator it;
    for (it = currentCache.begin(); it != currentCache.end(); it++){
        EV << (*it).getAgent() <<","<< (*it).getAddress() <<","<< (*it).getTimestamp() <<","<< (*it).getValue() << endl;
    }
}

NewscastCacheEntry NewscastCache::getRandomEntry(){
    NewscastCacheEntry ret;

    if (currentCache.size() > 0){
        int aRandomIndex = (int)intrand(currentCache.size());
        ret = currentCache.at(aRandomIndex);
    }

    return ret;
}

NewscastCacheEntry NewscastCache::getEntry(IPvXAddress addr){
    NewscastCacheEntry ret;

    for (unsigned int i = 0; i < currentCache.size(); i++)
        if (currentCache.at(i).getAddress().equals(addr)){
            ret = currentCache.at(i);
            break;
        }

    return ret;
}

std::vector<IPvXAddress> NewscastCache::getAllAddresses(){
    std::vector<IPvXAddress> ret;

    CacheSet::iterator it;
    for (it = currentCache.begin(); it != currentCache.end(); it++)
        if (!(*it).getAddress().isUnspecified())
            ret.push_back( (*it).getAddress());

    return ret;
}


long NewscastCache::getEstimatedSizeInBits(){

    long ret = 0;

    CacheSet::iterator it;
    for (it = currentCache.begin(); it != currentCache.end(); it++){
        ret += (*it).getEstimatedSizeInBits();
    }

    return ret;

}
