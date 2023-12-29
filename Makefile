INSTALL_DIR = /usr/local/bin

tarsau: tarsau.c    
	gcc tarsau.c -o tarsau

install: tarsau
	cp tarsau $(INSTALL_DIR)

clean:                          
	rm *.o tarsau
