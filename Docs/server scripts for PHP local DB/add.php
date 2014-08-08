<?php
   include("conec.php");
   $link=Conection();
$Sql="insert into arduino_environment (temp1,moi1)  values ('".$_GET["temp1"]."', '".$_GET["moi1"]."')";     
   mysql_query($Sql,$link);
   header("Location: monitor.php");
?>