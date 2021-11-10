// Image server 
// Open up a net socket on port 3333 and waits for connection.
// When a client has connected the server will expect to recieve jpeg images with meta data. 
// If meta data is pressent it will be annotated on image. 
//
// Written by Jørgen Kragh Jakobsen, October 2021, Copenhagen Demnark 
// 
// As is 
// 

package main

import (
	"net"
	"fmt"
	"io"
	"bufio"
	"bytes"
	"github.com/fogleman/imview"
	"image"
	"image/color"
	"image/jpeg"
)

var (
    img image.Image 
	score uint8
	ll,lr,ul,ur int
	col color.Color
)

func main() {
	fmt.Printf("Start socket server on port: 3333  .................")
	ln, _ := net.Listen("tcp", ":3333")           // Setup port 
    conn, _ := ln.Accept()
	fmt.Printf("Connected\n")

	request_image_cmd := []byte{1,1,0} 
	msg               := make([]byte,8)
		
	for j:=1;j<100;j++ { 
		fmt.Printf("Loop %d : Send request image command (1,1) ..............",j)
		
		conn.Write(request_image_cmd)
		c    := bufio.NewReader(conn)
		n, _ := c.Read(msg)  // Blocking 
        fmt.Printf("n:%d\n",n); 

		var meta = []byte{0} 
		var msg_size uint32
		if n == 8 { 
			msg_type  := msg[0]  
			msg_size = uint32(msg[1])<<16 + uint32(msg[2])<<8 + uint32(msg[3])
			meta_size := uint32(msg[5])<<16 + uint32(msg[6])<<8 + uint32(msg[7])
			fmt.Printf("Got message type : %d , Size : %d , Meta data : %d\n",msg_type, msg_size,meta_size)
			if meta_size > 0 { 
				meta = make([]byte,meta_size)
				_ , _ = c.Read(meta);  
			    
				score = meta[3];
				ll    = int(uint16(meta[4])<<8 + uint16(meta[5]));
				lr    = int(uint16(meta[6])<<8 + uint16(meta[7]));
				ul    = int(uint16(meta[8])<<8 + uint16(meta[9]));
				ur    = int(uint16(meta[10])<<8 + uint16(meta[11]));
			    fmt.Printf("Score: %d  BB: (%d, %d) (%d, %d)\n", score, ll, lr, ul , ur)
			}
		} else { 
			fmt.Printf("Fail to get message \n")
		}
		if msg_size != 0 { 
		 
		 	buff  := make([]byte,0,msg_size) 
			n, _ = io.ReadFull(c,buff[:cap(buff)])
			buff = buff[:n]
         
			fmt.Printf("Read full image,  size : %d \n",len(buff));
			img, _  = jpeg.Decode(bytes.NewReader(buff));
		  
			 // Annotate
			
			imgRGB := imview.ImageToRGBA(img)
			Rect(ll,lr,ul,ur,2,imgRGB)  
		
		    imview.Show( imgRGB)
	    }
    } 
}

func Rect(x1, y1, x2, y2, thickness int, img *image.RGBA) {
    col := color.RGBA{0, 255, 0, 255}
    for t:=0; t<thickness; t++ {
        // draw horizontal lines
        for x := x1; x<= x2; x++ {
            img.Set(x, y1+t, col)
            img.Set(x, y2-t, col)
        }
        // draw vertical lines
        for y := y1; y <= y2; y++ {
            img.Set(x1+t, y, col)
            img.Set(x2-t, y, col)
        }
    }
}