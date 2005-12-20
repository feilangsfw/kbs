<?php
	require("www2-funcs.php");
	login_init();

	$brdarr = array();
	if( isset( $_GET["bid"] ) ){
		$brdnum = $_GET["bid"] ;
		settype($brdnum,"integer");
		if( $brdnum == 0 ){
			html_error_quit("�����������!");
		}
		$board = bbs_getbname($brdnum);
		if( !$board ){
			html_error_quit("�����������");
		}
		if( $brdnum != bbs_getboard($board, $brdarr) ){
			html_error_quit("�����������");
		}
	}
	elseif (isset($_GET["board"])){
		$board = $_GET["board"];
		$brdnum = bbs_getboard($board, $brdarr);
		if ($brdnum == 0) {
			html_error_quit("�����������");
		}
	}
	elseif (isset($_SERVER['argv'])){
		$board = $_SERVER['argv'][1];
		$brdnum = bbs_getboard($board, $brdarr);
		if ($brdnum == 0) {
			html_error_quit("�����������");
		}
	}
	else {
		html_error_quit("�����������");
	}
	$isnormalboard=bbs_normalboard($board);
	bbs_set_onboard($brdnum,1);

	$usernum = $currentuser["index"];

	if (!$isnormalboard && bbs_checkreadperm($usernum, $brdnum) == 0) {
		html_error_quit("�����������");
	}
	if (isset($_GET["id"]))
		$id = $_GET["id"];
	elseif (isset($_SERVER['argv'][2]))
		$id = $_SERVER['argv'][2];
	else {
		html_error_quit("��������º�");
	}
	settype($id, "integer");
	// ��ȡ��һƪ����һƪ��ͬ������һƪ����һƪ��ָʾ
	@$ptr=$_GET["p"];
	// ͬ�����ָʾ�����ﴦ��
	if ($ptr == "tn")
	{
		$articles = bbs_get_threads_from_id($brdnum, $id, $dir_modes["NORMAL"],1);
		if ($articles == FALSE)
			$redirt_id = $id;
		else
			$redirt_id = $articles[0]["ID"];
		if (($loginok == 1) && $currentuser["userid"] != "guest")
			bbs_brcaddread($brdarr["NAME"], $redirt_id);
		header("Location: " . "bbscon.php?bid=" . $brdnum . "&id=" . $redirt_id);
		exit;
	}
	elseif ($ptr == "tp")
	{
		$articles = bbs_get_threads_from_id($brdnum, $id, $dir_modes["NORMAL"],-1);
		if ($articles == FALSE)
			$redirt_id = $id;
		else
			$redirt_id = $articles[0]["ID"];
		if (($loginok == 1) && $currentuser["userid"] != "guest")
			bbs_brcaddread($brdarr["NAME"], $redirt_id);
		header("Location: " . "bbscon.php?bid=" . $brdnum . "&id=" . $redirt_id);
		exit;
	}

	if (isset($_GET["ftype"])){
		$ftype = intval($_GET["ftype"]);
	} else {
		$ftype = $dir_modes["NORMAL"];
	}
	$dir_perm = bbs_is_permit_mode($ftype, 1);
	if (!$dir_perm) {
		html_error_quit("�����ģʽ");
	}

	if(($ftype == $dir_modes["DELETED"]) && (!bbs_is_bm($brdnum, $usernum)))
	{
		html_error_quit("�㲻�ܿ��������Ӵ��");
	}
	
	$total = bbs_countarticles($brdnum, $ftype);
	if ($total <= 0) {
		html_error_quit("��������º�,ԭ�Ŀ����Ѿ���ɾ��");
	}
	$articles = array ();

    if ($dir_perm == 1) { //sorted
        $articles = array ();
        $num = bbs_get_records_from_id($brdarr["NAME"], $id, $ftype, $articles);
        if ($num <= 0) html_error_quit("��������º�,ԭ�Ŀ����Ѿ���ɾ��");
        $article = $articles[1];
    } else {
        $num = @intval($_GET["num"]);
        if (($num <= 0) || ($num > $total)) html_error_quit("��������º�,ԭ�Ŀ����Ѿ���ɾ��");
        if (($articles = bbs_getarticles($brdarr["NAME"], $num, 1, $ftype)) === false) html_error_quit("��������º�,ԭ�Ŀ����Ѿ���ɾ��");
        if ($id != $articles[0]["ID"]) html_error_quit("��������º�,ԭ�Ŀ����Ѿ���ɾ��");
        $article = $articles[0];
    }
	$filename = bbs_get_board_filename($board, $article["FILENAME"]);
	if ($isnormalboard && ($ftype != $dir_modes["DELETED"])) {
		if (cache_header("public",@filemtime($filename),300))
			return;
	}

	@$attachpos=$_GET["ap"];//pointer to the size after ATTACHMENT PAD
	if ($attachpos!=0) {
		require_once("attachment.php");
		output_attachment($filename, $attachpos);
		exit;
	}

	if (!$ftype && $ptr == 'p' && $articles[0]["ID"] != 0) {
		if ($currentuser["userid"] != "guest")
			bbs_brcaddread($brdarr["NAME"], $articles[0]["ID"]);
		header("Location: " . "bbscon.php?bid=" . $brdnum . "&id=" . $articles[0]["ID"]);
		exit;
	}
	if (!$ftype && $ptr == 'n' && $articles[2]["ID"] != 0)
	{
		if ($currentuser["userid"] != "guest")
			bbs_brcaddread($brdarr["NAME"], $articles[2]["ID"]);
		header("Location: " ."bbscon.php?bid=" . $brdnum . "&id=" . $articles[2]["ID"]);
		exit;
	}
	page_header("�Ķ�����".$dir_name[$ftype], "<a href=\"bbsdoc.php?board=".$brdarr["NAME"]."\">".htmlspecialchars($brdarr["DESC"])."</a>");
?>
<h1><?php echo $brdarr["NAME"]; ?> �� <?php echo $dir_name[$ftype]; ?></h1>
<script>
var o = new conWriter(<?php echo $ftype; ?>, '<?php echo addslashes($brdarr["NAME"]); ?>', <?php echo $brdnum; ?>, <?php
echo $article["ID"];?>, <?php echo $article["GROUPID"];?>, <?php echo $article["REID"];?>, '<?php echo $article["FILENAME"];?>', '<?php
echo addslashes(bbs_get_super_fav($article['TITLE'], "bbscon.php?bid=" . $brdnum . "&id=" . $article["ID"]));?>', <?php echo $num; ?>);
o.h(1);
attachURL = 'bbscon.php?<?php echo $_SERVER["QUERY_STRING"]; ?>';
<?php echo bbs2_readfile($filename); ?>
o.h(0);o.t();</script>
<?php
	if (($ftype==0) && ($loginok==1) && ($currentuser["userid"] != "guest"))
		bbs_brcaddread($brdarr["NAME"], $articles[1]["ID"]);
	page_footer();
?>