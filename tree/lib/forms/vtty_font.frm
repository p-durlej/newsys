form(-1, -1, 160, 74, "Font");
flag(FORM_FRAME);
flag(FORM_TITLE);
flag(FORM_ALLOW_CLOSE);

chkbox("normal", 16, 16, "Normal");
chkbox("narrow", 80, 16, "Narrow");

button(NULL, 16, 40, 60, 20, "OK", 1);
button(NULL, 80, 40, 60, 20, "Cancel", 1);
