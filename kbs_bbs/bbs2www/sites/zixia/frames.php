<?php
	if (!isset($_GET['boardid'])){
		$url='bbssec.php';
	} else {
		$url='bbsdoc.php?board='.$_GET['boardid'];
	}
?>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=gb2312">
<title>��ӭݰ��BBS������վ</title>
</head>
<script src="/browser.js"></script>
<script language=javascript>
if(is_ie) document.write(""+
"  <frameset name=mainframe frameborder=0 border=0 cols=\"180,*\">"+
        "<frame name=menu scrolling=no src=menu.html>"+
        "<frameset name=viewfrm rows=*,20>"+
              "<frame framespacing=\"2\" name=\"f3\" src=\"/<?php echo $url; ?>\">"+
              "<frame scrolling=no marginwidth=4 marginheight=\"1\" framespacing=\"1\" name=\"f4\" src=\"bbsguestfoot.php\">");
else document.write(""+
"  <frameset cols=\"180,*\" frameborder=0 border=0 >"+
    "<frame name=\"f2\" framespacing=\"0\" src=\"/cgi-bin/bbs/bbsleft\">"+
    "<frameset name=\"viewfrm\" rows=\"*, 20\">"+
      "<frame framespacing=\"2\" name=\"f3\" src=\"/<?php echo $url; ?>\">"+
      "<frame scrolling=\"no\" marginwidth=\"4\" marginheight=\"1\" framespacing=\"1\" name=\"f4\" src=\"bbsguestfoot.php\">");
</script>
    </frameset>
  </frameset>
</html>