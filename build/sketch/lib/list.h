#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\list.h"
#ifndef DR_LIST_H
#define DR_LIST_H

#include "./logger.h"

namespace DevRelief {

Logger LinkedListLogger("LinkedList",LINKED_LIST_LOGGER_LEVEL);
Logger PtrListLogger("PtrList",PTR_LIST_LOGGER_LEVEL);

template<class T>
struct ListNode
{
	T data;
	ListNode<T> *next;
};


template <typename T>
class LinkedList{

public:
	LinkedList();
	virtual ~LinkedList();


	virtual int size() const;

	virtual bool insertAt(int index, T);

	virtual bool add(T);
    virtual T get(int index) const;
    virtual void removeAt(int index);
    virtual void removeFirst(T t);
    virtual void removeAll(T t);
    virtual int firstIndexOf(T t,int start=0) const;
    virtual T last() const;
    virtual void clear();
    T operator[](int index) const  { return this->get(index); }
	//T operator[](size_t& i)  const { return this->get(i); }
  	//const T operator[](const size_t& i) const { return this->get(i); }
    

    void each(auto&& lambda) const;
    T* first(auto&& lambda) const;

protected:
    virtual void deleteNode(ListNode<T>* node);
    virtual ListNode<T>* getNode(int index) const;
	int m_size;
	ListNode<T> *m_root;
	ListNode<T>	*m_last;
    Logger * m_logger;

};



template<typename T>
LinkedList<T>::LinkedList()
{
    m_logger = &LinkedListLogger;
	m_root=NULL;
	m_last=NULL;
	m_size=0;
}

template<typename T>
LinkedList<T>::~LinkedList()
{
    m_logger->debug("delete LinkedList node");

	ListNode<T>* tmp;
	while(m_root!=NULL)
	{
        m_logger->debug("\tdelete node");
		tmp=m_root;
		m_root=m_root->next;
		deleteNode(tmp);
	}

	m_size=0;

}

template<typename T>
void LinkedList<T>::each(auto&& lambda) const {
    m_logger->debug("iterate list");
  ListNode<T>* node = m_root;
  while(node != NULL) {
      m_logger->debug("\thandle item 0x%04X  --> 0x%04X",node,node->next);
      lambda(node->data);
      m_logger->debug("\tdone");
      node = node->next;
  }  
}


template<typename T>
T* LinkedList<T>::first(auto&& lambda) const {
    m_logger->debug("find first match");
    ListNode<T>* node = m_root;
    ListNode<T>* matchNode = NULL;
    while(node != NULL && matchNode == NULL) {
        m_logger->debug("\tcompare item 0x%04X  --> 0x%04X",node,node->next);
        if (lambda(node->data)) {
            matchNode = node;
        }
        node = node->next;
    }  
    return matchNode ? &(matchNode->data) : NULL;
}

template<typename T>
T LinkedList<T>::get(int index) const{
    ListNode<T>* node = getNode(index);
    if (node != NULL) {
        return node->data;
    }
    m_logger->info("LinkedList get index %d returned NULL",index);
    return NULL;
}

template<typename T>
ListNode<T>* LinkedList<T>::getNode(int index) const{
    m_logger->debug("getNode %d (size=%d)",index,m_size);
    if (index < 0 || index >= size()){
        return NULL;
    }
	int pos = 0;
	ListNode<T>* current = m_root;

	while(pos < index && current){
		current = current->next;

		pos++;
	}

	// Check if the object index got is the same as the required
	if(pos == index){
		return current;
	}

	return NULL;
}

template<typename T>
int LinkedList<T>::size() const{
	return m_size;
}

template<typename T>
bool LinkedList<T>::insertAt(int index, T item){
    m_logger->debug("insert at %d (size=%d)",index,m_size);

	ListNode<T> *tmp = new ListNode<T>();
	tmp->data = item;
	tmp->next = NULL;
	
    if (m_root == NULL) {
        m_logger->debug("\t root");
        m_root = tmp;
    } else if (index == 0) {
        m_logger->debug("\t before root");
        tmp->next = m_root;
        m_root = tmp;
    } else {
        m_logger->debug("\t get prev node");
        ListNode<T>* prev = getNode(index-1);
        if (prev == NULL) {
            prev = getNode(m_size-1);
            m_logger->debug("\tinsert past end.  use last");
        }
        m_logger->debug("\t got prev node");
        tmp->next = prev->next;
        prev->next = tmp;
    }

    m_size++;
	return true;
}

template<typename T>
bool LinkedList<T>::add(T item){
	ListNode<T> *tmp = new ListNode<T>();
    m_logger->debug("Add Node 0x%04X",tmp);
	tmp->data = item;
	tmp->next = NULL;
	
    if (m_root == NULL) {
        m_root = tmp;
    } else {
        ListNode<T>* node = m_root;
        while(node->next != NULL) {
            node = node->next;
        }
        node->next = tmp;
    }

	m_size++;

	return true;
}

template<typename T>
T LinkedList<T>::last() const{
    if (m_root == NULL) {
        return T();
    }
    ListNode<T>* node = m_root;
    while(node->next != NULL) {
        node = node->next;
    }
    return node->data;
}

template<typename T>
void LinkedList<T>::clear(){
    while(m_root != NULL){
        ListNode<T>*next = m_root->next;
        deleteNode(m_root);
        m_root = next;
    }
    m_size = 0;
}

template<typename T>
void LinkedList<T>::removeAt(int index){
    m_logger->never("remove at %d",index);
	if (index < 0 || index >= m_size)
	{
		return;
	}
    if (index == 0) {
        m_logger->never("remove root");
        ListNode<T>*next = m_root->next;
        m_logger->never("delete root");
        deleteNode(m_root);
        m_logger->never("deleted");
        m_root = next;
        m_size--;
        return;
    }

    m_logger->never("get node %d",index-1);

	ListNode<T>*prev = getNode(index-1);
    m_logger->never("got 0x%04X",prev);
    ListNode<T>*tmp = prev->next;
    m_logger->never("tmp 0x%04X",tmp);
    prev->next = tmp->next;
    m_logger->never("prev->next 0x%04X",prev->next);
    m_size --;
    m_logger->never("delete 0x%04X",tmp);
    deleteNode(tmp);
    m_logger->never("deleted 0x%04X",tmp);
}

template<typename T>
void LinkedList<T>::removeFirst(T t) {
    m_logger->never("remove first");
    removeAt(firstIndexOf(t));
    m_logger->never("\tremoved");
}

template<typename T>
void LinkedList<T>::removeAll(T t){
    m_logger->debug("removeAll %d",t);
    int idx = firstIndexOf(t);
    while(idx>=0) {
        m_logger->debug("\t index %d",idx);
        LinkedList<T>::removeAt(idx);
        idx = firstIndexOf(t);
    }
}

template<typename T>
int LinkedList<T>::firstIndexOf(T t,int start) const{
    m_logger->never("firstIndexOf %d after %d",t,start);
    int idx = start;
    ListNode<T>*node = m_root;
    while(idx<m_size&&node != NULL && node->data != t) {
        node = node->next;
        idx++;
    }
    m_logger->never("result 0x%04x  --  %d",node,idx);
    return node != NULL ? idx : -1;
}

template<typename T>
void LinkedList<T>::deleteNode(ListNode<T>* t){
    m_logger->debug("primitive deleteNode");
    delete t;
}

template<typename T>
class PtrList : public LinkedList<T> {
    public:
    	PtrList();
	    virtual ~PtrList();

    protected:
       // virtual ListNode<T> getNodePtr(int idx);
        virtual void deleteNode(ListNode<T>*node);
};


template<typename T>
PtrList<T>::~PtrList(){
    PtrListLogger.debug("~PtrList() start");
    ListNode<T>*node=LinkedList<T>::getNode(0);
    while(node != NULL) {
        LinkedList<T>::m_logger->debug("\tdelete node");
        ListNode<T>*next = node->next;
        deleteNode(node);
        LinkedList<T>::m_logger->debug("\tdeleted node");
        node = next;
        LinkedList<T>::m_logger->debug("\tnext 0x%0X",node);
    }
    LinkedList<T>::m_root = NULL;
    PtrListLogger.debug("~PtrList() done");

}

template<typename T>
PtrList<T>::PtrList(){
    LinkedList<T>::m_logger = &PtrListLogger;
}
/*
template<typename T>
ListNode<T> PtrList<T>::getNodePtr(int idx) {
    return LinkedList<T>::getNode(idx);
}
*/
template<typename T>
void PtrList<T>::deleteNode(ListNode<T>*node) {
    LinkedList<T>::m_logger->debug("delete PtrList node 0x%04X",node);
    if (node == 0) {
        LinkedList<T>::m_logger->error("PtrList has NULL node");
    } else {
        LinkedList<T>::m_logger->debug("\tdelete PtrList node data 0x%04X",node->data);
        delete node->data;
        LinkedList<T>::m_logger->debug("\tdelete PtrList node 0x%04X",node);
        delete node;
    }
}

};
#endif