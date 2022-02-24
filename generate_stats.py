if __name__ == '__main__':

    sent_packets = 0
    acked_bytes = 0
    time = 0

    with open("./TCP/events.log","r") as f:
        for line in f:
            parse = line.strip().split()
            event = parse[2]
            if event == 'transmitting':
                sent_packets += 1
            elif event == 'client': # packet correctly received
                acked_bytes += int(parse[-2])
            time = int(parse[1])

    print("throughput = {} bytes/sec".format(acked_bytes/time))