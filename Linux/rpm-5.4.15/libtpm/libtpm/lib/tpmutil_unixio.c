/********************************************************************************/
/*			     	TPM Utility Functions				*/
/*			     Written by Kenneth Goldman, Stefan Berger		*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpmutil_unixio.c 4089 2010-06-09 00:50:31Z kgoldman $	*/
/********************************************************************************/

#include "copyright.h"

#include "system.h"

/* These are platform specific.  This version uses a UnixIO socket interface.

   Environment variables are:
           
   TPM_UNIXIO_PATH - the path to the UNIX IO socket
*/

#include <stdarg.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <tpmfunc.h>
#include <tpm_lowlevel.h>

#include "debug.h"

/****************************************************************************/
/*                                                                          */
/* Open the socket to the TPM Host emulation                                */
/*                                                                          */
/****************************************************************************/

static uint32_t TPM_OpenClientSocket_UnixIO(int *sock_fd)
{
    struct stat sb;
    char *unixio_path;

    unixio_path = getenv("TPM_UNIXIO_PATH");
    if (unixio_path == NULL) {
	printf
	    ("TPM_OpenClientSocket: Error, TPM_UNIXIO_PATH environment variable not set\n");
	return ERR_IO;
    }

    if (stat(unixio_path, &sb) == 0) {
	if (S_ISSOCK(sb.st_mode)) {
	    *sock_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	    if (*sock_fd > 0) {
		struct sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, unixio_path, sizeof(addr.sun_path));
		addr.sun_path[sizeof(addr.sun_path)-1] = '\0';
		if (connect(*sock_fd, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
		    return 0;
		} else {
		    close(*sock_fd);
		    *sock_fd = 0;
		}
	    }
	}
    }
    return ERR_IO;
}

/****************************************************************************/
/*                                                                          */
/* Close the socket to the TPM Host emulation                               */
/*                                                                          */
/****************************************************************************/

static uint32_t TPM_CloseClientSocket(int sock_fd)
{
    close(sock_fd);
    return 0;
}

/* write buffer to socket sock_fd */

static uint32_t TPM_TransmitSocket(int sock_fd, struct tpm_buffer *tb,
				   const char *msg)
{
    size_t nbytes = 0;
    ssize_t nwritten = 0;
    size_t nleft = 0;
    unsigned int offset = 0;
    char mymsg[1024];

    snprintf(mymsg, sizeof(mymsg), "TPM_TransmitSocket: To TPM [%s]", msg);

    nbytes = tb->used;

    showBuff(tb->buffer, mymsg);

    nleft = nbytes;
    while (nleft > 0) {
	nwritten = write(sock_fd, &tb->buffer[offset], nleft);
	if (nwritten < 0) {	/* error */
	    printf("TPM_TransmitSocket: write error %d\n", (int) nwritten);
	    return nwritten;
	}
	nleft -= nwritten;
	offset += nwritten;
    }
    return 0;
}

/* read nbytes from socket sock_fd and put them in buffer */

static uint32_t TPM_ReceiveBytes(int sock_fd,
				 unsigned char *buffer, size_t nbytes)
{
    int nread = 0;
    int nleft = nbytes;

    while (nleft > 0) {
	nread = read(sock_fd, buffer, nleft);
	if (nread < 0) {	/* error */
	    printf("TPM_ReceiveBytes: read error %d\n", nread);
	    return ERR_IO;
	} else if (nread == 0) {	/* EOF */
	    printf("TPM_ReceiveBytes: read EOF\n");
	    return ERR_IO;
	}
	nleft -= nread;
	buffer += nread;
    }
    return 0;
}

/* read a TPM packet from socket sock_fd */

static uint32_t TPM_ReceiveSocket(int sock_fd, struct tpm_buffer *tb)
{
    uint32_t rc = 0;
    uint32_t paramSize = 0;
    uint32_t addsize = 0;
    unsigned char *buffer = tb->buffer;

    if (TPM_LowLevel_Use_VTPM())
	addsize = sizeof(uint32_t);

    /* read the tag and paramSize */
    if (rc == 0)
	rc = TPM_ReceiveBytes(sock_fd, buffer,
			      addsize + TPM_U16_SIZE + TPM_U32_SIZE);

    /* extract the paramSize */
    if (rc == 0) {
	paramSize = LOAD32(buffer, addsize + TPM_PARAMSIZE_OFFSET);
	if (paramSize > TPM_MAX_BUFF_SIZE) {
	    printf
		("TPM_ReceiveSocket: ERROR: paramSize %u greater than %u\n",
		 paramSize, TPM_MAX_BUFF_SIZE);
	    rc = ERR_BAD_RESP;
	}
    }
    /* read the rest of the packet */
    if (rc == 0)
	rc = TPM_ReceiveBytes(sock_fd,
			      buffer + addsize + TPM_U16_SIZE +
			      TPM_U32_SIZE,
			      paramSize - (TPM_U16_SIZE + TPM_U32_SIZE));

    /* read the TPM return code from the packet */
    if (rc == 0) {
	showBuff(buffer, "TPM_ReceiveSocket: From TPM");
	rc = LOAD32(buffer, addsize + TPM_RETURN_OFFSET);
	tb->used = addsize + paramSize;
    }
    return rc;
}

static struct tpm_transport unixio_transport = {
    .open = TPM_OpenClientSocket_UnixIO,
    .close = TPM_CloseClientSocket,
    .send = TPM_TransmitSocket,
    .recv = TPM_ReceiveSocket,
};

void TPM_LowLevel_TransportUnixIO_Set(void)
{
    TPM_LowLevel_Transport_Set(&unixio_transport);
}
