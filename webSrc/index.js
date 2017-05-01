function check() {
    if ($('#username').val() == '') {
        $('#password-danger').text("用户名为空！");
        return false;
    } else if ($('#password').val() == '') {
        $('#password-danger').text("密码为空！");
        return false;
    } else {
        return true;
    };
}