#pragma once

/*
Fast 2048 int16_t circular buffer
Eng. Deulis Antonio Pelegrin Jaime
2020-06-17
*/

#define FastAudioFIFO_SIZE 8 //MUST BE POWER OF 2 !!!
#define FastAudioFIFO_MASK (FastAudioFIFO_SIZE-1)

SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
class FastAudioFIFO 
{
public:

	void init(void)
	{
		head_ = 0;
		tail_ = 0;
	}

	bool put(int16_t item)
	{
		// if (full())
		// 	return false;

		// std::lock_guard<std::mutex> lock(mutex_);
		int16_buf_[(head_++) & FastAudioFIFO_MASK] = item;

		return true;
	}

	bool put(uint8_t item)
	{
		// if (full())
		// 	return false;

		// std::lock_guard<std::mutex> lock(mutex_);
		uint8_buf_[(head_++) & FastAudioFIFO_MASK] = item;

		return true;
	}

	bool put_frame(uint8_t * item, int frame_size)
	{
		// if (full())
		// 	return false;

		// std::lock_guard<std::mutex> lock(mutex_);

		for(int i = 0; i < frame_size; i++){
		// xSemaphoreTake(mutex, portMAX_DELAY);
		frame_buf_[(head_) & FastAudioFIFO_MASK][i] = *(item+i);

		}
		head_++;
		// xSemaphoreGive(mutex);
		return true;
	}
	bool get(short* item)
	{
		// std::lock_guard<std::mutex> lock(mutex_);

		// if (empty())
		// 	return false;

		*item = int16_buf_[(tail_++) & FastAudioFIFO_MASK];

		return true;
	}

	bool get(uint8_t* item)
	{
		// std::lock_guard<std::mutex> lock(mutex_);

		// if (empty()) {
		// 	printf("EMPTY \n");
		// 	return false;
		// }

		*item = uint8_buf_[(tail_++) & FastAudioFIFO_MASK];

		return true;
	}

	bool get_frame(uint8_t* item, int frame_size)
	{
		// std::lock_guard<std::mutex> lock(mutex_);
		// if (empty())
		// 	return false;

		// xSemaphoreTake(mutex, portMAX_DELAY);
		for(int i = 0; i < frame_size; i++) {
		*(item+i) = frame_buf_[(tail_) & FastAudioFIFO_MASK][i];
		}
		tail_++;
		// xSemaphoreGive(mutex);
		return true;
	}
	
	void reset(void)
	{
		// std::lock_guard<std::mutex> lock(mutex_);
		head_ = tail_;
	}

	bool empty(void) const
	{
		//if head and tail are equal, we are empty
		return head_ == tail_;
	}

	bool full(void) const
	{
		//If tail is ahead the head by 1, we are full
		return ((head_ + 1) & FastAudioFIFO_MASK) == (tail_ & FastAudioFIFO_MASK);
	}

	size_t len(void) const
	{
		return head_ - tail_;
	}

	size_t available(void) const
	{
		return  FastAudioFIFO_SIZE - (head_ - tail_);
	}

private:
	// std::mutex mutex_;
	int16_t int16_buf_[FastAudioFIFO_SIZE];
	uint8_t uint8_buf_[FastAudioFIFO_SIZE];
	uint8_t frame_buf_[FastAudioFIFO_SIZE][8];
	size_t head_ = 0;
	size_t tail_ = 0;
};
