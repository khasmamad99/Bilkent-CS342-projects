all: isp producer consumer

isp: isp.c
	gcc -Wall -o isp isp.c -lrt

comsumer: consumer.c
	gcc -Wall -o consumer consumer.c -lrt

producer: producer.c
	gcc -Wall -o producer producer.c -lrt

clean:
	rm -fr *~ isp producer consumer