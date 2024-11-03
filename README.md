The core of this project is to implement a channel mechanism for synchronization between multiple threads through message passing. 
A channel is essentially a message queue/buffer with a fixed maximum capacity. 
A thread can act as a sender, sending messages by adding data to the queue/buffer of the channel, or as a receiver, receiving messages by removing data from the queue/buffer. 
Channels can be referenced and used by multiple senders and receivers at the same time.

In this lab, channels are used to support communication between multiple clients, allowing simultaneous read and write operations. 
We need to implement a functionally correct and efficient channel that supports multiple concurrency modes, such as blocking and non-blocking send/receive operations. 
In blocking mode, the receiver waits until data is available, while in non-blocking mode, the receiver returns immediately. 
For the sender, if the queue is full, the blocking mode waits until space is available, while the non-blocking mode returns directly.

We only needs to modify the following four files: channel.c, channel.h, and optionally linked_list.c and linked_list.h, and implement the following key functions:

channel_t* channel_create(size_t size): Create a new channel and specify its size.
enum channel_status channel_send(channel_t* channel, void* data): Send data to the channel, using blocking mode.
enum channel_status channel_receive(channel_t* channel, void** data): Receive data from the channel, using blocking mode.
enum channel_status channel_non_blocking_send(channel_t* channel, void* data): Send data in non-blocking mode.
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data): Receive data in non-blocking mode.
enum channel_status channel_close(channel_t* channel): Close the channel and reject further send operations.
enum channel_status channel_destroy(channel_t* channel): Destroys a channel and releases resources.
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index): Selects a ready channel from multiple channels.
