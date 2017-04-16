
form(-1, -1, 280, 102, "Format Diskette...");
flag(FORM_FRAME);
flag(FORM_TITLE);

bargraph("progbar", 8, 16, 266, 20);

label("status", 100, 44,  "Status: ready");
label("drive", 8, 44, "Drive: fd0,0");

frame(NULL, 8, 64, 266, 2, NULL);
button(NULL, 8, 76, 60, 20, "Format", 1);
button(NULL, 74, 76, 60, 20, "Drive...", 2);
button(NULL, 140, 76, 60, 20, "Close", 3);
