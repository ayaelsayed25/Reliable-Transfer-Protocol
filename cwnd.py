import matplotlib.pyplot as plt
if __name__ == '__main__':

    sent_packets = 0
    acked_bytes = 0
    time = 0
    cwnd = []
    x = []
    hello = 0
    with open("./TCP/events.log","r") as f:
        for line in f:
            parse = line.strip().split()
            event = parse[2]
            if event == 'window':
               x.append(hello)
               cwnd.append(int(parse[3]))
               hello +=1

    fig = plt.figure(1)  # identifies the figure
    plt.title("CWND", fontsize='16')  # title
    plt.plot(x, cwnd)  # plot the points
    plt.savefig('delay_plot.png')  # saves the figure in the present directory
    plt.grid()  # shows a grid under the plot
    plt.show()