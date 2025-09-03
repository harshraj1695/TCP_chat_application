const WebSocket = require("ws");
const net = require("net");

// TCP connection to your C server
const TCP_HOST = "127.0.0.1";
const TCP_PORT = 8080;

// WebSocket server for browsers
const wss = new WebSocket.Server({ port: 3000 });

wss.on("connection", (ws) => {
  console.log("Browser connected");

  // new TCP client for each browser
  const tcpClient = new net.Socket();
  tcpClient.connect(TCP_PORT, TCP_HOST, () => {
    console.log("Connected to C chat server");
  });

  // browser → C server
  ws.on("message", (msg) => {
    tcpClient.write(msg.toString());
  });

  // C server → browser
  tcpClient.on("data", (data) => {
    ws.send(data.toString());
  });

  // cleanup
  ws.on("close", () => {
    tcpClient.end();
  });
});
