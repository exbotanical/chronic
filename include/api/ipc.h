#ifndef IPC_H
#define IPC_H

/**
 * Initializes the IPC server and listens on the daemon's domain socket.
 */
void ipc_init(void);

/**
 * Shuts down the IPC server, closes fds, and deallocates associated resources.
 */
void ipc_shutdown(void);

#endif /* IPC_H */
