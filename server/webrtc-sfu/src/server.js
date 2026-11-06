/**
 * 文档方案 SFU：每客户端 1 条 WebRTC 连接到本服务；服务端转发音频轨（选择性转发）。
 * 信令：独立 WebSocket（与游戏 guandan_server 分离）。
 */
import wrtc from '@roamhq/wrtc';
import { WebSocketServer } from 'ws';

const { RTCPeerConnection, RTCSessionDescription, RTCIceCandidate } = wrtc;
const PORT = Number(process.env.SFU_PORT || 4444);

/** @type {Map<string, Map<string, PeerEntry>>} */
const rooms = new Map();

/**
 * @typedef {object} PeerEntry
 * @property {import('ws').WebSocket} ws
 * @property {RTCPeerConnection} pc
 * @property {string} playerId
 */

function getRoom(roomId) {
  let room = rooms.get(roomId);
  if (!room) {
    room = new Map();
    rooms.set(roomId, room);
  }
  return room;
}

function send(ws, payload) {
  if (ws.readyState === ws.OPEN) {
    ws.send(JSON.stringify(payload));
  }
}

async function renegotiateWithClient(peer) {
  const offer = await peer.pc.createOffer();
  await peer.pc.setLocalDescription(offer);
  send(peer.ws, {
    type: 'offer',
    sdp: peer.pc.localDescription.sdp,
    sdp_type: peer.pc.localDescription.type,
  });
}

function forwardTrackToRoom(roomId, fromPlayerId, track, stream) {
  const room = rooms.get(roomId);
  if (!room) return;
  for (const [pid, peer] of room) {
    if (pid === fromPlayerId) continue;
    try {
      peer.pc.addTrack(track, stream);
      renegotiateWithClient(peer).catch((e) => console.error('renegotiate', e));
    } catch (e) {
      console.error('addTrack failed', pid, e);
    }
  }
}

function removePeer(roomId, playerId) {
  const room = rooms.get(roomId);
  if (!room) return;
  const peer = room.get(playerId);
  if (peer) {
    try {
      peer.pc.close();
    } catch (_) {}
    room.delete(playerId);
  }
  if (room.size === 0) {
    rooms.delete(roomId);
  }
}

const wss = new WebSocketServer({ port: PORT });
console.log(`[SFU] WebRTC SFU ws://0.0.0.0:${PORT}`);

wss.on('connection', (ws) => {
  let roomId = null;
  let playerId = null;
  /** @type {PeerEntry | null} */
  let self = null;

  ws.on('message', async (raw) => {
    let msg;
    try {
      msg = JSON.parse(raw.toString());
    } catch {
      return;
    }

    try {
      if (msg.type === 'join') {
        roomId = msg.room_id;
        playerId = String(msg.player_id);
        const room = getRoom(roomId);
        const pc = new RTCPeerConnection({
          iceServers: [{ urls: 'stun:stun.l.google.com:19302' }],
        });

        pc.onicecandidate = (event) => {
          if (event.candidate) {
            send(ws, {
              type: 'candidate',
              candidate: event.candidate.candidate,
              sdp_mid: event.candidate.sdpMid,
              sdp_mline_index: event.candidate.sdpMLineIndex,
            });
          }
        };

        pc.ontrack = (event) => {
          forwardTrackToRoom(roomId, playerId, event.track, event.streams[0]);
        };

        self = { ws, pc, playerId };
        room.set(playerId, self);
        send(ws, { type: 'joined', player_id: playerId });
        return;
      }

      if (!self || !roomId) return;

      if (msg.type === 'offer') {
        await self.pc.setRemoteDescription(
          new RTCSessionDescription({ type: msg.sdp_type || 'offer', sdp: msg.sdp }),
        );
        const answer = await self.pc.createAnswer();
        await self.pc.setLocalDescription(answer);
        send(ws, {
          type: 'answer',
          sdp: answer.sdp,
          sdp_type: answer.type,
        });
        return;
      }

      if (msg.type === 'answer') {
        await self.pc.setRemoteDescription(
          new RTCSessionDescription({ type: msg.sdp_type || 'answer', sdp: msg.sdp }),
        );
        return;
      }

      if (msg.type === 'candidate') {
        await self.pc.addIceCandidate(
          new RTCIceCandidate({
            candidate: msg.candidate,
            sdpMid: msg.sdp_mid,
            sdpMLineIndex: msg.sdp_mline_index,
          }),
        );
      }
    } catch (e) {
      console.error('[SFU] message error', e);
      send(ws, { type: 'error', message: String(e) });
    }
  });

  ws.on('close', () => {
    if (roomId && playerId) {
      removePeer(roomId, playerId);
    }
  });
});
