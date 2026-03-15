/*
{
    userId: {
        type: mongoose.Schema.Types.ObjectId,
        ref: "User",
        required: true
    },

    timeArray: {
        type: Array,
        required: true
    },

    status_Array: {
        type: String,
        enum: ['completed', 'incomplete', 'late'],
        default: 'incomplete'
    },
    index: {
        type: Number,
        required: true, 
        default: 0
        },

    createdAt: {
        type: Date,
        default: Date.now
    },
    updatedAt: {
        type: Date,
        default: Date.now
    }
}
*/

import mongoose from 'mongoose';

const scheduleSchema = new mongoose.Schema({
    userId: {
        type: mongoose.Schema.Types.ObjectId,
        ref: "User",
        required: true
    },
    timeArray: {
        type: [String],
        required: true
    },
    status_Array: {
        type: [String],
        enum: ['completed', 'incomplete', 'skipped', 'late','empty'],
        default: 'empty'
    },
    index: {
        type: Number,
        required: true,
        default: 0
    },
    createdAt: {
        type: Date,
        default: Date.now
    },
    updatedAt: {
        type: Date,
        default: Date.now
    }
}, { timestamps: true });

const scheduleModel = mongoose.model("Schedule", scheduleSchema);
export default scheduleModel;
