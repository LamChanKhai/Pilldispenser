// Kiểm tra authentication và redirect nếu chưa đăng nhập
function checkAuth() {
    const token = localStorage.getItem('token');
    
    if (!token) {
        // Lưu URL hiện tại để redirect lại sau khi đăng nhập
        const currentPath = window.location.pathname;
        if (currentPath !== '/login.html' && currentPath !== '/signup.html' && !currentPath.includes('login.html') && !currentPath.includes('signup.html')) {
            sessionStorage.setItem('redirectAfterLogin', currentPath);
        }
        
        // Redirect đến trang đăng nhập
        window.location.href = 'login.html';
        return false;
    }
    
    return true;
}

// Lấy token từ localStorage
function getToken() {
    return localStorage.getItem('token');
}

// Lưu token vào localStorage
function setToken(token) {
    localStorage.setItem('token', token);
}

// Xóa token và đăng xuất
function logout() {
    localStorage.removeItem('token');
    window.location.href = 'login.html';
}

// Kiểm tra xem user đã đăng nhập chưa (không redirect)
function isAuthenticated() {
    return !!localStorage.getItem('token');
}

// Lấy headers với token để gửi request
function getAuthHeaders() {
    const token = getToken();
    return {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${token}`
    };
}

// Kiểm tra ngay khi script được load
// Chỉ kiểm tra nếu không phải trang login hoặc signup
const currentPage = window.location.pathname;
if (!currentPage.includes('login.html') && !currentPage.includes('signup.html')) {
    checkAuth();
}

