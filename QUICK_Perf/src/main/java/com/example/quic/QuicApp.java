package com.example.quic;

import java.net.InetSocketAddress;
import java.util.concurrent.TimeUnit;

import io.netty.bootstrap.Bootstrap;
import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;
import io.netty.channel.Channel;
import io.netty.channel.ChannelHandler;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelInboundHandlerAdapter;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.ChannelInputShutdownReadComplete;
import io.netty.channel.socket.nio.NioDatagramChannel;
import io.netty.handler.codec.LineBasedFrameDecoder;
import io.netty.handler.ssl.util.InsecureTrustManagerFactory;
import io.netty.handler.ssl.util.SelfSignedCertificate;
import io.netty.incubator.codec.quic.InsecureQuicTokenHandler;
import io.netty.incubator.codec.quic.QuicChannel;
import io.netty.incubator.codec.quic.QuicClientCodecBuilder;
import io.netty.incubator.codec.quic.QuicServerCodecBuilder;
import io.netty.incubator.codec.quic.QuicSslContext;
import io.netty.incubator.codec.quic.QuicSslContextBuilder;
import io.netty.incubator.codec.quic.QuicStreamChannel;
import io.netty.incubator.codec.quic.QuicStreamType;
import io.netty.util.CharsetUtil;
import io.netty.util.NetUtil;

public class QuicApp {

    public static void main(String[] args) {
        if (args.length < 1) {
            System.out.println("usage: java -jar quic-app.jar <mode>");
            System.exit(1);
        }

        String mode = args[0];
        try {
            if ("server".equalsIgnoreCase(mode)) {
                runServer(4433);
            } else if ("client".equalsIgnoreCase(mode)) {
                var host = "127.0.0.1";
                var port = 4433;
                var totalBytes = 10 * 1024 * 1024; // 10 MB
                var rateMbps = 10; // 10 Mbps
                runClient(host, port, totalBytes, rateMbps);
            } else {
                System.err.println("unknown mode: " + mode);
            }
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
        }
    }

    static void runServer(int port) throws Exception {
        System.out.println("Starting QUIC server on port " + port);
        SelfSignedCertificate selfSignedCertificate = new SelfSignedCertificate();
        QuicSslContext context = QuicSslContextBuilder.forServer(
                selfSignedCertificate.privateKey(), null, selfSignedCertificate.certificate())
                .applicationProtocols("http/0.9").build();
        NioEventLoopGroup group = new NioEventLoopGroup(1);

        ChannelHandler codec = new QuicServerCodecBuilder().sslContext(context)
                .maxIdleTimeout(5000, TimeUnit.MILLISECONDS)
                // Configure some limits for the maximal number of streams (and the data) that we want to handle.
                .initialMaxData(10000000)
                .initialMaxStreamDataBidirectionalLocal(1000000)
                .initialMaxStreamDataBidirectionalRemote(1000000)
                .initialMaxStreamsBidirectional(100)
                .initialMaxStreamsUnidirectional(100)
                .activeMigration(true)
                // Setup a token handler. In a production system you would want to implement and provide your custom
                // one.
                .tokenHandler(InsecureQuicTokenHandler.INSTANCE)
                // ChannelHandler that is added into QuicChannel pipeline.
                .handler(new ChannelInboundHandlerAdapter() {
                    @Override
                    public void channelActive(ChannelHandlerContext ctx) {
                        QuicChannel channel = (QuicChannel) ctx.channel();
                        // Create streams etc..
                    }

                    public void channelInactive(ChannelHandlerContext ctx) {
                        ((QuicChannel) ctx.channel()).collectStats().addListener(f -> {
                            if (f.isSuccess()) {
                                System.out.println("Connection closed: " + f.getNow());
                            }
                        });
                    }

                    @Override
                    public boolean isSharable() {
                        return true;
                    }
                })
                .streamHandler(new ChannelInitializer<QuicStreamChannel>() {
                    @Override
                    protected void initChannel(QuicStreamChannel ch) {
                        // Add a LineBasedFrameDecoder here as we just want to do some simple HTTP 0.9 handling.
                        ch.pipeline().addLast(new LineBasedFrameDecoder(1024))
                                .addLast(new ChannelInboundHandlerAdapter() {
                                    @Override
                                    public void channelRead(ChannelHandlerContext ctx, Object msg) {
                                        ByteBuf byteBuf = (ByteBuf) msg;
                                        try {
                                            if (byteBuf.toString(CharsetUtil.US_ASCII).trim().equals("GET /")) {
                                                ByteBuf buffer = ctx.alloc().directBuffer();
                                                buffer.writeCharSequence("Hello World!\r\n", CharsetUtil.US_ASCII);
                                                // Write the buffer and shutdown the output by writing a FIN.
                                                ctx.writeAndFlush(buffer).addListener(QuicStreamChannel.SHUTDOWN_OUTPUT);
                                            }
                                        } finally {
                                            byteBuf.release();
                                        }
                                    }
                                });
                    }
                }).build();

        try {
            Bootstrap bs = new Bootstrap();
            Channel channel = bs.group(group)
                    .channel(NioDatagramChannel.class)
                    .handler(codec)
                    .bind(new InetSocketAddress(9999)).sync().channel();
            channel.closeFuture().sync();
        } finally {
            group.shutdownGracefully();
        }
    }

    static void runClient(String host, int port, long totalBytes, double rateMbps) {
        System.out.printf("Client connecting to %s:%d%n", host, port);
        QuicSslContext context = QuicSslContextBuilder.forClient().trustManager(InsecureTrustManagerFactory.INSTANCE).
                applicationProtocols("http/0.9").build();
        NioEventLoopGroup group = new NioEventLoopGroup(1);
        try {
            ChannelHandler codec = new QuicClientCodecBuilder()
                    .sslContext(context)
                    .maxIdleTimeout(5000, TimeUnit.MILLISECONDS)
                    .initialMaxData(10000000)
                    .initialMaxStreamDataBidirectionalLocal(1000000)
                    .build();

            Bootstrap bs = new Bootstrap();
            Channel channel = bs.group(group)
                    .channel(NioDatagramChannel.class)
                    .handler(codec)
                    .bind(0).sync().channel();

            QuicChannel quicChannel = QuicChannel.newBootstrap(channel)
                    .streamHandler(new ChannelInboundHandlerAdapter() {
                        @Override
                        public void channelActive(ChannelHandlerContext ctx) {
                            // As we did not allow any remote initiated streams we will never see this method called.
                            // That said just let us keep it here to demonstrate that this handle would be called
                            // for each remote initiated stream.
                            ctx.close();
                        }
                    })
                    .remoteAddress(new InetSocketAddress(NetUtil.LOCALHOST4, 9999))
                    .connect()
                    .get();

            QuicStreamChannel streamChannel = quicChannel.createStream(QuicStreamType.BIDIRECTIONAL,
                    new ChannelInboundHandlerAdapter() {
                @Override
                public void channelRead(ChannelHandlerContext ctx, Object msg) {
                    ByteBuf byteBuf = (ByteBuf) msg;
                    System.err.println(byteBuf.toString(CharsetUtil.US_ASCII));
                    byteBuf.release();
                }

                @Override
                public void userEventTriggered(ChannelHandlerContext ctx, Object evt) {
                    if (evt == ChannelInputShutdownReadComplete.INSTANCE) {
                        // Close the connection once the remote peer did send the FIN for this stream.
                        ((QuicChannel) ctx.channel().parent()).close(true, 0,
                                ctx.alloc().directBuffer(16)
                                        .writeBytes(new byte[]{'k', 't', 'h', 'x', 'b', 'y', 'e'}));
                    }
                }
            }).sync().getNow();
            // Write the data and send the FIN. After this its not possible anymore to write any more data.
            streamChannel.writeAndFlush(Unpooled.copiedBuffer("GET /\r\n", CharsetUtil.US_ASCII))
                    .addListener(QuicStreamChannel.SHUTDOWN_OUTPUT);

            // Wait for the stream channel and quic channel to be closed (this will happen after we received the FIN).
            // After this is done we will close the underlying datagram channel.
            streamChannel.closeFuture().sync();
            quicChannel.closeFuture().sync();
            channel.close().sync();
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
        } finally {
            group.shutdownGracefully();
        }
    }
}
