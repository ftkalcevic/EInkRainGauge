#pragma once

template<typename T, int MAX_EVENTS> class Queue
{
	T Events[MAX_EVENTS];
	int Head;
	int Tail;
	
public:
	Queue()
	{
		Head = 0;
		Tail = 0;
	}
	
	void AddEvent(T event)
	{
		if (IsFull())
			return;
		Events[Tail] = event;
		Tail = (Tail + 1) % MAX_EVENTS;
	}
	
	T GetEvent()
	{
		if (Head == Tail)
			return 0;
		T event = Events[Head];
		Head = (Head + 1) % MAX_EVENTS;
		return event;
	}
	
	bool IsEmpty()
	{
		return Head == Tail;
	}
	
	bool IsFull()
	{
		return (Tail + 1) % MAX_EVENTS == Head;
	}
};

