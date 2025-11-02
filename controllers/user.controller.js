import User from "../model/user.model.js";

// Tạo user mới
export const createUser = async (req, res) => {
    try {
        const { name, email, phone, age, gender } = req.body;
        
        // Kiểm tra email đã tồn tại chưa
        const existingUser = await User.findOne({ email });
        if (existingUser) {
            return res.status(400).json({ error: "Email already exists" });
        }
        
        const user = new User({
            name,
            email,
            phone,
            age,
            gender
        });
        
        await user.save();
        res.status(201).json(user);
    } catch (error) {
        console.error("Error creating user:", error);
        res.status(500).json({ error: "Internal server error" });
    }
};

// Lấy tất cả users
export const getUsers = async (req, res) => {
    try {
        const users = await User.find().sort({ createdAt: -1 });
        res.json(users);
    } catch (error) {
        console.error("Error getting users:", error);
        res.status(500).json({ error: "Internal server error" });
    }
};

// Lấy user theo ID
export const getUserById = async (req, res) => {
    try {
        const { userId } = req.params;
        const user = await User.findById(userId);
        
        if (!user) {
            return res.status(404).json({ error: "User not found" });
        }
        
        res.json(user);
    } catch (error) {
        console.error("Error getting user:", error);
        res.status(500).json({ error: "Internal server error" });
    }
};

// Cập nhật user
export const updateUser = async (req, res) => {
    try {
        const { userId } = req.params;
        const { name, email, phone, age, gender } = req.body;
        
        // Kiểm tra email có trùng với user khác không
        if (email) {
            const existingUser = await User.findOne({ email, _id: { $ne: userId } });
            if (existingUser) {
                return res.status(400).json({ error: "Email already exists" });
            }
        }
        
        const user = await User.findByIdAndUpdate(
            userId,
            { name, email, phone, age, gender, updatedAt: Date.now() },
            { new: true, runValidators: true }
        );
        
        if (!user) {
            return res.status(404).json({ error: "User not found" });
        }
        
        res.json(user);
    } catch (error) {
        console.error("Error updating user:", error);
        res.status(500).json({ error: "Internal server error" });
    }
};

// Xóa user
export const deleteUser = async (req, res) => {
    try {
        const { userId } = req.params;
        const user = await User.findByIdAndDelete(userId);
        
        if (!user) {
            return res.status(404).json({ error: "User not found" });
        }
        
        res.json({ message: "User deleted successfully" });
    } catch (error) {
        console.error("Error deleting user:", error);
        res.status(500).json({ error: "Internal server error" });
    }
};

