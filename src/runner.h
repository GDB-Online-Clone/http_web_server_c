#pragma once

/**
 * @brief Represents a web server with routing and status information.
 * 
 * This structure holds the routing table, HTTP status code, and response body
 * for a web server.
 * 
 */
struct web_server {
    /**
     * @brief Pointer to the routing table containing the routes for the web server.
     */
    struct routes *route_table;
    /**
     * @brief The HTTP status code representing the current status of the web server.
     */
    int port_num;
    /**
     * @brief Size of buffer in which used by `listen`.
     */
    int backlog;
    /**
     * @brief Size of threadpool: http request handlers threadpool
     */
    int threadpool_size;
};

/**
 * @brief Run the web server
 * @param server Web server configuration structure
 * @return 0 on success, -1 on error
 */
int run_web_server(struct web_server server);