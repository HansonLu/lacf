#ifndef BUFFERED_STREAM_H 
#define BUFFERED_STREAM_H 

#include <vector> 
#include <assert.h> 

#include  "ace/Refcounted_Auto_Ptr.h"
#include "ace/Synch.h"

class CBufferredStream 
{
public: 
	explicit CBufferredStream (size_t max_sz = 1024*1024) 
	{ 
		max_sz_ = max_sz;
        stream_.reserve(100000);
	}

	int  write(const char * data, size_t len) 
	{
		assert(data); 
		assert(len);

		if ( len > avail()) 
		{
			return -1;
		}

		stream_.insert(stream_.end (), data, data + len);
		return 0;
	}

	bool empty() const 
	{ 
		return stream_.empty();
	}

	size_t avail() const 
	{
		return max_sz_- stream_.size();
	}

	const char * data() const 
	{
		if (stream_.empty()) 
			return 0 ; 
		return  &*stream_.begin();
	}

	void skip(size_t len) 
	{
		assert(len <= stream_.size());

		stream_.erase(stream_.begin(), stream_.begin() + len);

	}

	int read(char buf [], size_t len) 
	{
		int sztor = len ;

		if (stream_.empty()) 
		{
			return 0;
		}

		if ( stream_.size ()  < len) 
		{
			sztor = stream_.size();
		}

		memcpy(buf, &*(stream_.begin()), sztor);
		return sztor;
	}

	 size_t size() const 
	{ 
		return stream_.size();

	}

private : 
	std::vector<char> stream_;
	size_t max_sz_; 

};

typedef ACE_Refcounted_Auto_Ptr<CBufferredStream, ACE_Null_Mutex> BufferedStreamRefPtr; 


#endif 