import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

@Component({
  selector: 'app-single-pic',
  templateUrl: './single-pic.component.html',
  styleUrls: ['./single-pic.component.scss'],
})
export class SinglePicComponent implements OnInit {
  localUrl = null;
  isDragging = false;

  constructor() { }

  ngOnInit(): void {
  }

  onFileSelect(selectFileEvent: any) {
    if (selectFileEvent.target.files && selectFileEvent.target.files[0]) {
      this.previewImage(selectFileEvent.target.files[0]);
    }
  }

  onFileDrop(imageEvent: any) {
    const imageFile = imageEvent.dataTransfer.files[0];
    if (!imageFile.type.match(/image.*/)) {
      return;
    }
    this.previewImage(imageFile);
  }

  private previewImage(file: any): void {
    const reader = new FileReader();
    reader.onload = (event) => {
      this.localUrl = event.target.result;
      // this.render(this.localUrl);
    },
      reader.readAsDataURL(file);
  }


  render(src) {
    const image = new Image();
    image.onload = () => {
      const canvas = document.getElementById('myCanvas') as HTMLCanvasElement;
      /*     if (image.height > this.MAX_HEIGHT) {
            image.width *= this.MAX_HEIGHT / image.height;
            image.height = this.MAX_HEIGHT;
          } */
      const ctx = canvas.getContext('2d');
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      canvas.width = image.width;
      canvas.height = image.height;
      ctx.drawImage(image, 0, 0, image.width, image.height);
    };
    image.src = src;
  }

}
