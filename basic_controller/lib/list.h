#ifndef DR_LIST_H
#define DR_LIST_H

#include "./logger.h"

namespace DevRelief {

Logger ListLogger("List",DEBUG_LEVEL);

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


	virtual int size();

	virtual bool insertAt(int index, T);

	virtual bool add(T);
    virtual T get(int index);
    virtual void remove(int index);
    virtual T last();
    virtual void clear();
    T& operator[](int index); 
	T& operator[](size_t& i) { return this->get(i); }
  	const T& operator[](const size_t& i) const { return this->get(i); }
protected:
    virtual void deleteNode(ListNode<T>* node);
    virtual ListNode<T>* getNode(int index);
	int m_size;
	ListNode<T> *m_root;
	ListNode<T>	*m_last;
    Logger * m_logger;

};



template<typename T>
LinkedList<T>::LinkedList()
{
    m_logger = &ListLogger;
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
T LinkedList<T>::get(int index){
    ListNode<T>* node = getNode(index);
    if (node != NULL) {
        return node->data;
    }
    return NULL;
}

template<typename T>
ListNode<T>* LinkedList<T>::getNode(int index){
    m_logger->debug("getNode %d (size=%d)",index,m_size);
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
int LinkedList<T>::size(){
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
T LinkedList<T>::last(){
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
void LinkedList<T>::remove(int index){
	if (index < 0 || index >= m_size)
	{
		return;
	}


	ListNode<T>*prev = getNode(index-1);
    ListNode<T>*tmp = prev->next;
    prev->next = tmp->next;
    deleteNode(tmp);
    m_size --;
}

template<typename T>
void LinkedList<T>::deleteNode(ListNode<T>* t){
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
    ListLogger.debug("~PtrList");
    ListNode<T>*node=LinkedList<T>::getNode(0);
    while(node != NULL) {
        ListLogger.debug("\tdelete data");
        ListNode<T>*next = node->next;
        deleteNode(node);
        node = next;
    }
    LinkedList<T>::m_root = NULL;
}

template<typename T>
PtrList<T>::PtrList(){

}
/*
template<typename T>
ListNode<T> PtrList<T>::getNodePtr(int idx) {
    return LinkedList<T>::getNode(idx);
}
*/
template<typename T>
void PtrList<T>::deleteNode(ListNode<T>*node) {
    LinkedList<T>::m_logger->debug("delete PtrList node");
    delete node->data;
    delete node;
}

};
#endif