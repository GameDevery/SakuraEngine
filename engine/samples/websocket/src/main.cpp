#include "WebSocketSample/web_socket_cmds.hpp"

int main(int argc, char* argv[])
{
    using namespace skr;

    CmdParser          parser;
    WebSocketSampleCli cli;
    parser.main_cmd(
        &cli,
        {
            .help  = u8"WebSocketSample - a sample for WebSocket",
            .usage = u8"WebSocketSample [subcmd] [options]",
        }
    );
    parser.parse(argc, argv);

    return 0;
}