<?php
function Conection(){
   if (!($link=mysql_connect("localhost","skmadmin_user","g3b94r67")))  {
      exit();
   }
   if (!mysql_select_db("skmadmin_arduino",$link)){
      exit();
   }
   return $link;
}
?>