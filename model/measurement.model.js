import mongoose from 'mongoose';

const measurementSchema = new mongoose.Schema({
    userId: {
        type: mongoose.Schema.Types.ObjectId,
        ref: "User",
        required: true
    },
    heart_beat: {
        type: Number,
        required: true
    },
    spo2: {
        type: Number,
        required: true
    },
    temp: {
        type: Number,
        required: true
    },
    createdAt: {
        type: Date,
        default: Date.now
    }
}, { timestamps: true });

const Measurement = mongoose.model("Measurement", measurementSchema);
export default Measurement;
