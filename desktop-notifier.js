const admin = require('firebase-admin');
const notifier = require('node-notifier');
const path = require('path');

// ── Logger Utility ───────────────────────────────────────────────────────────
const log = {
  info: (msg) => console.log(`\x1b[36m[INFO]\x1b[0m   ${msg}`),
  success: (msg) => console.log(`\x1b[32m[OK]\x1b[0m     ${msg}`),
  warn: (msg) => console.log(`\x1b[33m[WARN]\x1b[0m   ${msg}`),
  error: (msg, err = '') => console.error(`\x1b[31m[ERROR]\x1b[0m  ${msg}`, err),
  system: (msg) => console.log(`\x1b[35m[SYSTEM]\x1b[0m ${msg}`),
  event: (msg) => console.log(`\x1b[34m[EVENT]\x1b[0m  ${msg}`)
};
// ============================================================================
// STANDALONE PRESENCE MONITOR
// Run this on any PC: node desktop-notifier.js
// It will give you native desktop notifications when users come online/offline
// ============================================================================

// ── 1. Load Service Account ──────────────────────────────────────────────────
let serviceAccount;
try {
  // Try to load from local file in the same directory
  serviceAccount = require('./obsidian-firebase.json');
} catch (error) {
  log.error("Could not find the Firebase Service Account JSON file.");
  log.error("Please make sure 'obsidian-firebase.json' is in the same folder as this script.");
  process.exit(1);
}

// ── 2. Initialize Firebase Admin ─────────────────────────────────────────────
if (!admin.apps.length) {
  admin.initializeApp({
    credential: admin.credential.cert(serviceAccount),
    databaseURL: 'https://obsidian-8234e-default-rtdb.firebaseio.com'
  });
}

const db = admin.database();
const knownStates = {};

log.system('STANDALONE PRESENCE MONITOR STARTED');
log.success('Connected to Firebase securely.');
log.info('Watching for online/offline events...');

// ── 3. Watch Presence ────────────────────────────────────────────────────────
db.ref('presence').on('value', (snapshot) => {
  const data = snapshot.val();
  if (!data || typeof data !== 'object') return;

  for (const [user, info] of Object.entries(data)) {
    if (!info || typeof info !== 'object') continue;

    // Skip initial load spam by checking if prev is undefined, 
    // but we WANT the initial state so we know who is already online when we start it.
    const state = info.state || 'offline';
    const device = info.device || 'Unknown';
    const prev = knownStates[user];

    if (prev !== state) {
      knownStates[user] = state;
      const time = new Date().toLocaleTimeString();

      // We only notify if it's a change or if they are already online when we start the script
      if (state === 'online') {
        const msg = `${user} is ONLINE on ${device}`;
        log.event(`ONLINE     |   ${user.padEnd(18)} | Device: ${device} | Time: ${time}`);
        
        // Native Desktop Notification
        notifier.notify({
          title: 'Chat Monitor',
          message: msg,
          sound: true, 
          wait: false
        });
      } else {
        const msg = `${user} went OFFLINE`;
        log.event(`OFFLINE    |   ${user.padEnd(18)} | Time: ${time}`);
        
        // Only trigger desktop notification for offline if it wasn't the first load
        if (prev !== undefined) {
          notifier.notify({
            title: 'Chat Monitor',
            message: msg,
            sound: false,
            wait: false
          });
        }
      }
    }
  }
}, (err) => {
  log.error('Firebase connection error:', err.message);
});
