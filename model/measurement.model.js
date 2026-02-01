import mongoose from 'mongoose';

const bpSchema = new mongoose.Schema({
    userId: {
        type: mongoose.Schema.Types.ObjectId,
        ref: "User",
        required: true
    },
    heart_beat: {
        type: Number,
        required: true
    },
    systolic : {
        type: Number,
        required: true
    },
    diastolic : {
        type: Number,
        required: true
    },
    createdAt: {
        type: Date,
        default: Date.now
    }
}, { timestamps: true });

const spo2Schema = new mongoose.Schema({
    userId: {
        type: mongoose.Schema.Types.ObjectId,
        ref: "User",
        required: true
    },
    spo2: {
        type: Number,
        required: true
    },
    temperature: {
        type: Number,
        required: true
    },
    createdAt: {
        type: Date,
        default: Date.now
    }
}, { timestamps: true });
const bpModel = mongoose.model("bp", bpSchema);
const spo2Model = mongoose.model("spo2", spo2Schema);
export { bpModel, spo2Model };

