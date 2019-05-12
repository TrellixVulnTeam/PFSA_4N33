from kivy.app import App
from kivy.uix.widget import Widget
from kivy.core.image import Image
import kivy
from kivy.animation import Animation
from kivy.app import App
from kivy.uix.button import Button
import os




class TestApp(App):

    def animate(self, instance):
        # create an animation object. This object could be stored
        # and reused each call or reused across different widgets.
        # += is a sequential step, while &= is in parallel
        animation = Animation(pos=(0, 550), t='linear')
        animation += Animation(pos=(0, 550), t='linear')
        animation &= Animation(size=(50, 50))
        animation += Animation(size=(50, 50))

        # apply the animation on the button, passed in the "instance" argument
        # Notice that default 'click' animation (changing the button
        # color while the mouse is down) is unchanged.
        animation.start(instance)

    def build(self):
        # create a button, and  attach animate() method as a on_press handler
        button = Button(size_hint=(0.1, 0.1), text="file",
                        on_press=self.animate)
        return button


if __name__ == '__main__':
    TestApp().run()
