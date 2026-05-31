#!/usr/bin/env node
/**
 * Test script to simulate ESP32 WebSocket connection
 * Run: node test-esp32-connection.js
 */

const WebSocket = require('ws');

const WS_URL = 'ws://localhost:3000/esp32';  // Change to wss://saayem.qzz.io/esp32 for production

console.log('╔════════════════════════════════════════╗');
console.log('║  ESP32 WebSocket Connection Test      ║');
console.log('╚════════════════════════════════════════╝\n');

console.log(`Connecting to: ${WS_URL}\n`);

const ws = new WebSocket(WS_URL);

ws.on('open', () => {
  console.log('✓ WebSocket Connected!\n');
  
  // Send ESP32 hello message
  const hello = {
    type: 'ESP32_HELLO',
    device: 'ESP32-Test'
  };
  
  console.log('→ Sending:', JSON.stringify(hello, null, 2));
  ws.send(JSON.stringify(hello));
});

ws.on('message', (data) => {
  console.log('\n← Received:');
  try {
    const msg = JSON.parse(data);
    console.log(JSON.stringify(msg, null, 2));
    
    if (msg.type === 'PRESENCE_UPDATE') {
      console.log('\n╔════════════════════════════════════════╗');
      console.log('║       PRESENCE UPDATE                  ║');
      console.log('╠════════════════════════════════════════╣');
      console.log(`║ User:   ${msg.user.padEnd(30)} ║`);
      console.log(`║ State:  ${msg.state.padEnd(30)} ║`);
      console.log(`║ Device: ${msg.device.padEnd(30)} ║`);
      console.log(`║ Time:   ${msg.time.padEnd(30)} ║`);
      console.log('╚════════════════════════════════════════╝\n');
    }
  } catch (e) {
    console.log(data.toString());
  }
});

ws.on('close', () => {
  console.log('\n⚠ WebSocket Disconnected');
  process.exit(0);
});

ws.on('error', (error) => {
  console.error('\n✗ WebSocket Error:', error.message);
  process.exit(1);
});

// Keep alive
process.on('SIGINT', () => {
  console.log('\n\nClosing connection...');
  ws.close();
});

console.log('Waiting for messages... (Press Ctrl+C to exit)\n');
