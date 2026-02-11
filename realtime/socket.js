let ioInstance = null;

export function setIO(io) {
  ioInstance = io;
}

export function getIO() {
  return ioInstance;
}

/**
 * Safe emit that becomes a no-op when Socket.IO
 * isn't running (e.g. on Vercel serverless).
 */
export function emit(event, payload) {
  if (!ioInstance) return;
  ioInstance.emit(event, payload);
}

