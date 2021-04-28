#include "scout/loaders/tcp_client_loader.h"
#include "scout/tcp_server.h"
#include "scout/pack.h"
#include "scout/pic/pic_wrapper.h"

/* External API function */
extern void flush_cache(uint8_t * buffer, uint32_t size);

void main()
{
    sock_fd clientSock;
    int32_t status;
    uint32_t size;
    uint8_t * receiveBuffer;

#ifdef SCOUT_RESTORE_FLOW
    clientSock = 0;
    receiveBuffer = NULL;
#endif /* SCOUT_RESTORE_FLOW */

    /* Open the TCP server */
    status = connect_to_tcp_server(&clientSock, SCOUT_SERVER_IP, SCOUT_LOADER_PORT);
    if (status != STATUS_OK)
    {
        goto free_resources;
    }

    /* Receive the allocation size */
    if (full_net_recv(clientSock, (uint8_t*)&size, sizeof(size)) != sizeof(size))
    {
        goto free_resources;
    }

    size = ntohl(size);

#ifdef SCOUT_MMAP
    receiveBuffer = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
#else /* !SCOUT_MMAP */
    /* Allocate the receive buffer */
    receiveBuffer = (uint8_t*)malloc(size);
#endif /* SCOUT_MMAP */
    if (receiveBuffer == NULL)
    {
        goto free_resources;
    }

    /* Receive the loaded content */
    if (full_net_recv(clientSock, receiveBuffer, size) != size)
    {
        goto free_resources;
    }

    /* Flush the cache for this buffer */
    flush_cache(receiveBuffer, size);

#ifdef SCOUT_MMAP
    mprotect(receiveBuffer, size, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif /* SCOUT_MMAP */

    /* Jump into the buffer */
#if defined(SCOUT_LOADING_THUMB_CODE)
    ((void (*)(void))receiveBuffer + 1)();
#else
    ((void (*)(void))receiveBuffer)();
#endif /* SCOUT_LOADING_THUMB_CODE */

free_resources:
#ifdef SCOUT_RESTORE_FLOW
    /* Free the buffer */
    if (receiveBuffer != NULL)
    {
#ifdef SCOUT_MMAP
        munmap(receiveBuffer, size);
#else /* !SCOUT_MMAP */
        free(receiveBuffer);
#endif /* SCOUT_MMAP */
    }

    /* Close the socket */
    if (clientSock > 0)
    {
        close(clientSock);
    }
#endif /* SCOUT_RESTORE_FLOW */
    return;
}
