static void LCD_DispAscii16x16(unsigned int x0, unsigned int y0,unsigned int  color,const unsigned char ch[])
{
        unsigned short int i,j;
        unsigned char buffer = 0;
        for(i=0;i<32;i++)
        {
                buffer=ch[i];
                for(j=0;j<8;j++)
                {
                      //  if(buffer&mask)
                      	if(buffer&(0x01<<j))
                        {
                                //lcd_draw_pixel(x0+j,y0+i,color);
								//lcd_draw_pixel(row+i, col+j, color);
								lcd_draw_pixel(y0+i, x0+j, color);
                        }
                              //  buffer=buffer>>1;
                }

                i++;
                buffer=ch[i];
                for(j=0;j<8;j++)
                {
                       // if(buffer&mask)
                       if(buffer&(0x01<<j))
                        {
                                //lcd_draw_pixel(x0+j+8, y0+i-1, color);
                                lcd_draw_pixel(y0+i-1, x0+j+8, color);
                        }
                               
                                //buffer=buffer>>1;
                }
        }
}

void lcd_draw_pixel(int row, int col, int color)
{
    unsigned long * pixel = (unsigned long  *)FB_ADDR;//LCD屏缓存区首地址
    *(pixel + row * COL + col) = color;
}
