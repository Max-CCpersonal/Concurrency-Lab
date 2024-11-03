#include "channel.h"
#include <pthread.h>

// Creates a new channel with the provided size and returns it to the caller
channel_t* channel_create(size_t size)
{
    /* IMPLEMENT THIS */
    // channel is a buffer that store the info, so i think it need a malloc to allocate some space for it.
    channel_t* new_channel = (channel_t*)malloc(sizeof(channel_t));
    if (new_channel == NULL){
        return NULL;
    }
    // so channel use buffer.c to create a buffer
    new_channel->buffer = buffer_create(size);
    // buffer_create is using malloc also.
    if(new_channel->buffer == NULL){
        free(new_channel);
        return NULL;
    }
    
    //init the mutex for thread lock.
    if (pthread_mutex_init(&new_channel->mutex,NULL)!= 0){
        buffer_free(new_channel->buffer);
        //if fail lock init, it should fail create and free all the things.
        free(new_channel);
        return NULL;
    }

    //init the cond vars for send and receive check.
    if(pthread_cond_init(&new_channel->ready_send,NULL) != 0){
        pthread_mutex_destroy(&new_channel->mutex);
        buffer_free(new_channel->buffer);
        free(new_channel);
        return NULL;
    }
    
    if(pthread_cond_init(&new_channel->ready_receive,NULL) != 0){
        pthread_cond_destroy(&new_channel->ready_send);
        pthread_mutex_destroy(&new_channel->mutex);
        buffer_free(new_channel->buffer);
        free(new_channel);
        return NULL;
    }

    new_channel->close_open_flag = 0;
    // After the first init, the buffer should be open and empty.
    return new_channel;
}

// Writes data to the given channel
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_send(channel_t *channel, void* data)
{
    /* IMPLEMENT THIS */
    
    //lock to block other to access.
    //mutex need to be set at every operations to let each operation to follow the lock rule.
    //if a locks it, b want to lock it, b will wait until a unlock it. then b lock it and do b's operation. 
    pthread_mutex_lock(&channel->mutex);

    // CLOSED_ERROR if the channel is closed
    if (channel->close_open_flag == 1){
        pthread_cond_broadcast(&channel->ready_send);
        pthread_cond_broadcast(&channel->ready_receive);
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    //Wait if full for buffer is not full
    //wait only the current number of element in buffer is same as buffer's capacity number.(That means FULL)
    //if buffer is not full, it will go to add directly and will not wait.
    while(buffer_current_size(channel->buffer) == buffer_capacity(channel->buffer)){

        //printf("send before\n");
        pthread_cond_wait(&channel->ready_send,&channel->mutex);
        //printf("send after\n");

        if (channel->close_open_flag == 1){
            pthread_cond_broadcast(&channel->ready_send);
            pthread_cond_broadcast(&channel->ready_receive);
            pthread_mutex_unlock(&channel->mutex);
            return CLOSED_ERROR;
        }
    }

    if (buffer_add(channel->buffer,data) == BUFFER_ERROR){
        pthread_cond_broadcast(&channel->ready_send);
        pthread_cond_broadcast(&channel->ready_receive);
        pthread_mutex_unlock(&channel->mutex);
        return GENERIC_ERROR;
    }

    // Wake up others that want to receive, it waits because the channel was empty, now it's not.
    pthread_cond_broadcast(&channel->ready_receive);

    pthread_mutex_unlock(&channel->mutex);

    return SUCCESS;
}

// Reads data from the given channel and stores it in the function's input parameter, data (Note that it is a double pointer)
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&channel->mutex);

    // CLOSED_ERROR if the channel is closed
    if (channel->close_open_flag == 1){
        pthread_cond_broadcast(&channel->ready_receive);
        pthread_cond_broadcast(&channel->ready_send);
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    //wait when buffer is empty

    while(buffer_current_size(channel->buffer) == 0){
        if (channel->close_open_flag == 1){
            pthread_cond_broadcast(&channel->ready_receive);
            pthread_cond_broadcast(&channel->ready_send);
            pthread_mutex_unlock(&channel->mutex);
            return CLOSED_ERROR;
        }

        //printf("recv before\n");
        pthread_cond_wait(&channel->ready_receive,&channel->mutex);
        //printf("recv after\n");

        if (channel->close_open_flag == 1){
            pthread_cond_broadcast(&channel->ready_receive);
            pthread_cond_broadcast(&channel->ready_send);
            pthread_mutex_unlock(&channel->mutex);
            return CLOSED_ERROR;
        }
    }

    if (buffer_remove(channel->buffer,data) == BUFFER_ERROR){
        //buffer_remove did put things into *data.
        pthread_cond_broadcast(&channel->ready_receive);
        pthread_cond_broadcast(&channel->ready_send);
        pthread_mutex_unlock(&channel->mutex);
        return GENERIC_ERROR;
    }

    // Wake up others that want to receive, it waits because the channel was empty, now it's not.
    pthread_cond_broadcast(&channel->ready_send);

    pthread_mutex_unlock(&channel->mutex);


    return SUCCESS;
}

// Writes data to the given channel
// This is a non-blocking call i.e., the function simply returns if the channel is full
// Returns SUCCESS for successfully writing data to the channel,
// CHANNEL_FULL if the channel is full and the data was not added to the buffer,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_send(channel_t* channel, void* data)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&channel->mutex);

    // CLOSED_ERROR if the channel is closed
    if (channel->close_open_flag == 1){
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    if (buffer_capacity(channel->buffer) == buffer_current_size(channel->buffer)){
        pthread_mutex_unlock(&channel->mutex);
        return CHANNEL_FULL;
    }

    if (buffer_add(channel->buffer,data) == BUFFER_ERROR){
        pthread_mutex_unlock(&channel->mutex);
        return GENERIC_ERROR;
    }

    // Wake up others that want to receive, it waits because the channel was empty, now it's not.
    pthread_cond_signal(&channel->ready_receive);

    pthread_mutex_unlock(&channel->mutex);

    return SUCCESS;
}

// Reads data from the given channel and stores it in the function's input parameter data (Note that it is a double pointer)
// This is a non-blocking call i.e., the function simply returns if the channel is empty
// Returns SUCCESS for successful retrieval of data,
// CHANNEL_EMPTY if the channel is empty and nothing was stored in data,
// CLOSED_ERROR if the channel is closed, and
// GENERIC_ERROR on encountering any other generic error of any sort
enum channel_status channel_non_blocking_receive(channel_t* channel, void** data)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&channel->mutex);

    // CLOSED_ERROR if the channel is closed
    if (channel->close_open_flag == 1){
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }
    
    if (buffer_current_size(channel->buffer) == 0){
        pthread_mutex_unlock(&channel->mutex);
        return CHANNEL_EMPTY;
    }

    if (buffer_remove(channel->buffer,data) == BUFFER_ERROR){
        //buffer_remove did put things into *data.
        pthread_mutex_unlock(&channel->mutex);
        return GENERIC_ERROR;
    }

    // Wake up others that want to receive, it waits because the channel was empty, now it's not.
    pthread_cond_signal(&channel->ready_send);

    pthread_mutex_unlock(&channel->mutex);

    return SUCCESS;
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// GENERIC_ERROR in any other error case
enum channel_status channel_close(channel_t* channel)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&channel->mutex);

    if(channel->close_open_flag == 1){
        pthread_mutex_unlock(&channel->mutex);
        return CLOSED_ERROR;
    }

    //change status first, and wake waiting ooperations
    //informs all the blocking send/receive/select calls to return with CLOSED_ERROR
    channel->close_open_flag = 1;

    pthread_cond_broadcast(&channel->ready_send);
    pthread_cond_broadcast(&channel->ready_receive);

    pthread_mutex_unlock(&channel->mutex);

    return SUCCESS;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// GENERIC_ERROR in any other error case
enum channel_status channel_destroy(channel_t* channel)
{
    /* IMPLEMENT THIS */
    pthread_mutex_lock(&channel->mutex);

    if(channel->close_open_flag != 1){
        pthread_mutex_unlock(&channel->mutex);
        return DESTROY_ERROR;
    }
    pthread_mutex_unlock(&channel->mutex);
    //do the opposite of init
    if (pthread_cond_destroy(&channel->ready_send) != 0){
        return GENERIC_ERROR;
    }
    if (pthread_cond_destroy(&channel->ready_receive) != 0){
        return GENERIC_ERROR;
    }
    
    pthread_mutex_destroy(&channel->mutex);

    buffer_free(channel->buffer);
    
    free(channel);

    return SUCCESS;
}

// Takes an array of channels (channel_list) of type select_t and the array length (channel_count) as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum channel_status channel_select(select_t* channel_list, size_t channel_count, size_t* selected_index)
{
    /* IMPLEMENT THIS */
    pthread_mutex_t select_lock;
    pthread_cond_t select_cond_var;
    if (pthread_mutex_init(&select_lock, NULL)){
        return GENERIC_ERROR;
    }
    if (pthread_cond_init(&select_cond_var, NULL)){
        pthread_mutex_destroy(&select_lock);
        return GENERIC_ERROR;
    }
    bool channel_avail = false;

    pthread_mutex_lock(&select_lock);

    while(channel_avail != true){
        //change previous function with the proper update on chaneel_avail status.
        if (channel_list->channel->close_open_flag == 1){
            pthread_cond_broadcast(&channel_list->channel->ready_receive);
            pthread_cond_broadcast(&channel_list->channel->ready_send);
            pthread_mutex_unlock(&select_lock);
            return CLOSED_ERROR;
        }

        //should combine the send and receive together for select.
        pthread_cond_wait(&channel_list->channel->ready_receive,&select_lock);

        if (channel_list->channel->close_open_flag == 1){
            pthread_cond_broadcast(&channel_list->channel->ready_receive);
            pthread_cond_broadcast(&channel_list->channel->ready_send);
            pthread_mutex_unlock(&select_lock);
            return CLOSED_ERROR;
        }
    }

    pthread_mutex_unlock(&select_lock);
    return SUCCESS;
}
